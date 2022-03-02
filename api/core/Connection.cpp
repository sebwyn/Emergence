#include "Connection.hpp"

#include <chrono>

#include "Logger.hpp"
#include "Protocol.hpp"

Connection::Connection(UdpSocket &socket) : socket(socket) {}

void Connection::onConnect() {
    // important because connection objects can potentially be reused
    receivedPackets.set();
}

void Connection::update() {
    auto updateStart = std::chrono::high_resolution_clock::now();

    if (station == disconnected) {
        return;
    }

    // we're either connecting or already connected
    // handle timeout
    auto deltaReceivedTime = std::chrono::duration_cast<std::chrono::seconds>(
                                 updateStart - lastPacketTime)
                                 .count();
    if (deltaReceivedTime > Globals::timeout) {
        if (station == connecting) {
            Logger::logInfo("The connection with " + other.ip + ":" +
                            std::to_string(other.port) + " timed out!");
        } else if (station == connected) {
            Logger::logInfo("Failed to connect to " + other.ip + ":" +
                            std::to_string(other.port));
            // TODO: send some kind of app event
        }
        station = disconnected;
    }

    // send a message to the client every second that keeps the
    // connection alive, and acks
    auto deltaSentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                             updateStart - lastSentTime)
                             .count();
    if (deltaSentTime > (1000 / Globals::goodSendRate)) {

        if (station == connecting) {
            // send the message in connectMessage
            std::vector<Protocol::AppData> toSend;
            toSend.push_back(
                Protocol::AppData(currentMessage, connectionMessage));
            send(toSend);
        } else if (station == connected) {
            std::vector<Protocol::AppData> toSend;
            for (auto message : messages) {
                toSend.push_back(Protocol::AppData(currentMessage++, message));
            }
            messages.clear();

            // rather than doing this latest message thing
            // I should just add a hook for when messages are being sent
            // and add the messages via that
            if (latestMessage.length()) {
                toSend.push_back(
                    Protocol::AppData(currentMessage++, latestMessage));
                latestMessage = "";
            }

            if (toSend.size() > 0) {
                send(toSend);
            } else {
                sendKeepAlive();
            }
        }
    }

    // TODO: update the mode
}

void Connection::connect(ConnectId _other, const std::string &message) {
    if (station == disconnected) {
        other = _other;
        connectionMessage = message;

        station = connecting;
        lastPacketTime = std::chrono::high_resolution_clock::now();
        localSeqNum = currentMessage = 0;
    }
}

void Connection::sendKeepAlive() {
    Protocol::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Protocol::Packet packet(header);

    sentPackets.insert(std::pair<uint, Protocol::PacketHandled>(
        header.seq,
        Protocol::PacketHandled(packet, [](Protocol::PacketHandled ph) {})));
    socket.sendTo(other.ip, other.port, packet.toBuffer());

    lastSentTime = std::chrono::high_resolution_clock::now();
}

void Connection::send(std::vector<Protocol::AppData> &messages,
                      std::function<void(Protocol::PacketHandled)> onResend) {
    Protocol::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Protocol::Packet packet(header, messages);

    sentPackets.insert(std::pair<uint, Protocol::PacketHandled>(
        header.seq, Protocol::PacketHandled(packet, onResend)));
    socket.sendTo(other.ip, other.port, packet.toBuffer());

    lastSentTime = std::chrono::high_resolution_clock::now();
}

// the only issue i see is this relies on us receiving packets in order
// to resend lost packets
void Connection::receive(Protocol::Packet packet) {
    lastPacketTime = std::chrono::high_resolution_clock::now();

    switch (station) {
    case connected:
        // process the message in the rest of the function
        break;
    case disconnected:
        // ignore messages if we're disconnected
        return;
    case connecting:
        onConnect();
        station = connected;
        break;
    }

    ack(packet);

    // iterate over sentpackets removing any packets from sentpackets that
    // have been acked, what remains, hasn't been acked and can be dealt with
    // after a second has passed
    for (auto it = sentPackets.cbegin(); it != sentPackets.cend();) {
        Protocol::PacketHandled packetHandled = it->second;
        // regardless of whether or not its been acked calculate time since
        // sending message
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - packetHandled.sent)
                           .count();
        if (acked(packet.header, it->first)) {
            if (!rttDefined) {
                rtt = elapsed;
                rttDefined = true;
            } else {
                rtt += Globals::rttSmooth * elapsed;
            }
            // if its been acked remove from list
            sentPackets.erase(it++);
            continue;
        } else {
            // if its been over resend time, resend the packet
            if (elapsed > Globals::packetLostTime) {
                // The packet was lost
                packetHandled.onResend(packetHandled);

                // remove from list
                sentPackets.erase(it++);
                continue;
            }
        }
        ++it;
    }

    for (const auto message : packet.messages) {
        receivedMessages.push_back(message);
    }
}

bool Connection::acked(Protocol::Header header, uint seq) {
    if (header.ack == seq) {
        return true;
    } else {
        if (seq < header.ack && header.ack - seq <= 32) {
            return receivedPackets.test(header.ack - seq - 1);
        }
    }
    return false;
}

void Connection::ack(Protocol::Packet packet) {
    // rotate our received bitset by the difference between remoteSeqNum and
    // receivedSeqNum then set the bitset so the previous remoteSeqNum is acked
    // keep in mind packets may arrive out of order so sequence number may be
    // less than our remote sequence number
    if (packet.header.seq > remoteSeqNum) {
        receivedPackets = receivedPackets << packet.header.seq - remoteSeqNum;
        // ack the previous remote seq num
        receivedPackets.set(packet.header.seq - remoteSeqNum - 1);
        remoteSeqNum = packet.header.seq;
    } else if (packet.header.seq < remoteSeqNum &&
               (remoteSeqNum - packet.header.seq) < 32) {
        receivedPackets.set(remoteSeqNum - packet.header.seq - 1);
    }
}

void Connection::updateMode() {
    auto now = std::chrono::high_resolution_clock::now();
    // time elapsed in mode
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - modeStart)
            .count();

    // check rtt, and if it is bad, switch to a bad sendrate
    if (mode == Globals::ConnectionState::GOOD && rtt > Globals::badRtt) {
        // immediately drop to bad mode
        mode = Globals::ConnectionState::BAD;
        modeStart = std::chrono::high_resolution_clock::now();

        Logger::logInfo("Entering bad mode!");

        if (elapsed < 10000) {
            returnToGood *= 2;
            if (returnToGood > 60000)
                returnToGood = 60000;
        }
    }

    if (mode == Globals::ConnectionState::BAD && elapsed > returnToGood) {

        Logger::logInfo("Entering good mode!");

        // return to good after returnToGood time has passed
        mode = Globals::ConnectionState::GOOD;
        lastTrustTime = modeStart = std::chrono::high_resolution_clock::now();
    }

    auto timeSinceTrust = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now - lastTrustTime)
                              .count();
    if (mode == Globals::ConnectionState::GOOD && timeSinceTrust > 10000) {
        // trust the connection
        returnToGood /= 2;
        lastTrustTime = std::chrono::high_resolution_clock::now();
        if (returnToGood < 1000)
            returnToGood = 1000;
    }
}
