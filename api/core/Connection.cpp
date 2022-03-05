#include "Connection.hpp"

#include <chrono>

#include "Logger.hpp"
#include "Protocol.hpp"

Connection::Connection(UdpSocket &socket) : socket(socket) {
    toSend.setValue(std::vector<std::string>(0));
    toSend.getValue().reserve(100);
}

void Connection::connect(ConnectId _other, const std::string &message) {
    if (station.load() == disconnected) {
        other = _other;
        connectionMessage = message;

        station.store(connecting);
        lastPacketTime = std::chrono::high_resolution_clock::now();
        localSeqNum = currentMessage = 0;
    }
}

//TODO: send disconnection message
void Connection::disconnect(){
    if(station.load() != disconnected){
        station.store(disconnected);
    }
}

// since conne
void Connection::onConnect() {
    // important because connection objects can potentially be reused
    // reserver space for 100 messages
    Logger::logInfo("Established connection");
    receivedPackets.set();
}

void Connection::execute() {
    while (station.load() != disconnected) {
        auto updateStart = std::chrono::high_resolution_clock::now();

        update();

        auto updateEnd = std::chrono::high_resolution_clock::now();

        // sleep for update time minus the update time
        auto deltaUpdate =
            std::chrono::duration_cast<std::chrono::microseconds>(updateEnd -
                                                                  updateStart);
        /*Logger::logInfo("Updating took: " +
                        std::to_string(deltaUpdate.count()) + " microseconds");
        */
        //std::cout << "Sleeping for: " << (std::chrono::microseconds(1000000 / fps.load()) - deltaUpdate).count() << std::endl;
        std::this_thread::sleep_for(std::chrono::microseconds(1000000 / fps.load()) -
                                    deltaUpdate);
    }
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
        if (station == connected) {
            Logger::logInfo("The connection with " + other.ip + ":" +
                            std::to_string(other.port) + " timed out!");
        } else if (station == connecting) {
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
            std::vector<Protocol::AppData> sending;

            toSend.access([&sending, this](std::vector<std::string> &data) {
                for (auto message : data) {
                    sending.push_back(
                        Protocol::AppData(currentMessage++, message));
                }
                data.clear();
            });

            if (sending.size() > 0) {
                send(sending);
            } else {
                sendKeepAlive();
            }
        }
    }

    //Logger::logInfo("Getting here");
    toReceive.access([this](std::vector<Protocol::Packet> &data){
        for(auto packet : data){
            receive(packet);
        }
        data.clear();
    });

    // TODO: update the mode
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

    //Logger::logInfo("sending: " + packet.toBuffer());

    sentPackets.insert(std::pair<uint, Protocol::PacketHandled>(
        header.seq, Protocol::PacketHandled(packet, onResend)));
    socket.sendTo(other.ip, other.port, packet.toBuffer());

    lastSentTime = std::chrono::high_resolution_clock::now();
}

// the only issue i see is this relies on us receiving packets in order
// to resend lost packets
void Connection::receive(Protocol::Packet packet) {
    lastPacketTime = std::chrono::high_resolution_clock::now();

    switch (station.load()) {
    case connected:
        // process the message in the rest of the function
        break;
    case disconnected:
        // ignore messages if we're disconnected
        return;
    case connecting:
        onConnect();
        station.store(connected);
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

    receivedFrom.access([&packet](std::vector<Protocol::AppData> &data) {
        for (const auto message : packet.messages) {
                data.push_back(Protocol::AppData(message));
                //Logger::logInfo("Received: " + message.message);
        }
    });
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
