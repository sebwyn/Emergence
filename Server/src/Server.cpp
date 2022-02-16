#include "Server.hpp"

#include "Globals.hpp"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <random>

Server::Server() : socket(AF_INET, SOCK_DGRAM) {
    socket.bind(Globals::port);
    socket.setNonBlocking();

    receivedPackets.set();
}

void Server::update(){

    //receive messages and ack
    lastSentTime = std::chrono::high_resolution_clock::now();
    while(message != "quit"){
        auto updateStart = std::chrono::high_resolution_clock::now();

        if(connectionEstablished){
            //timeout the connection after timeout length
            if(std::chrono::duration_cast<std::chrono::seconds>(updateStart - lastPacketTime).count() > Globals::timeout){
                std::cout << "The connection timed out!" << std::endl;
                break;
            }

            //send a message to the client every second that keeps the connection alive, and acks
            if(std::chrono::duration_cast<std::chrono::milliseconds>(updateStart - lastSentTime).count() > (1000 / Globals::goodSendRate)){
                sendKeepAlive(ip, clientPort);
                lastSentTime = updateStart;
            }
        }

        //receive a valid packet per our protocol, and establish a connection if 
        //one is not already established
        auto received = receive();
        if(received.has_value()){
            lastPacketTime = std::chrono::high_resolution_clock::now();

            //not a keep alive
            if(received->messages.size()){
                for(Globals::AppData message : received->messages){
                    std::cout << "C:" << message.toString() << std::endl;
                }
            }
        }

        //connection established check is contained within,
        //so input continues to get read even if there is no connection
        //however data is only sent if there is a connection
        sendInput();

        //all this update time stuff may be a little unnecessary because it takes less than a ms
        auto updateFinish = std::chrono::high_resolution_clock::now();
        auto updateTime = std::chrono::duration_cast<std::chrono::milliseconds>(updateFinish - updateStart).count();
        //std::cout << "updating took: " << updateTime << std::endl;
        //sleep for fps - updateTime so that we consistently update to maintain fps
        usleep( 1000 / Globals::fps - updateTime);
    }
}

void Server::sendKeepAlive(std::string ip, ushort port){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header);

    sentPackets.insert(std::pair<uint, Globals::PacketHandled>(header.seq, Globals::PacketHandled(packet, [](Globals::PacketHandled ph){})));
    socket.sendTo(ip, port, packet.toBuffer());
}

void Server::send(std::string ip, int port, std::vector<Globals::AppData>& messages, std::function<void(Globals::PacketHandled)> onResend){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header, messages);

    sentPackets.insert(std::pair<uint, Globals::PacketHandled>(header.seq, Globals::PacketHandled(packet, onResend)));
    socket.sendTo(ip, port, packet.toBuffer());
}

std::optional<Globals::Packet> Server::receive(){
    std::unique_ptr<char> received;
    auto from = socket.receiveFrom(&received);
    if(from.has_value()){
        //the message actually has content
        Globals::Packet packet(received);

        //ensure the message is using our protocol
        if(std::string(packet.header.protocol) != Globals::protocol())
            return {};

        if(!connectionEstablished){
            connectionEstablished = true;
            ip = from->first;
            clientPort = from->second;
            lastSentTime = std::chrono::high_resolution_clock::now();
        } else if(from->first != ip){
            //drop the packet if it isn't coming from the one client
            //TODO: make it so another connection is created
            return {};
        }

        //add a chance to drop packets for testing purposes
        std::random_device r;
        std::default_random_engine e1(r());
        std::uniform_real_distribution<double> distribution(0.);
        if(distribution(e1) < Globals::packetLoss){
            //std::cout << "Dropping packet: " << receivedPackets << std::endl;
            return {};
        }

        handlePacket(packet);

        return packet;
    }
    return {};
}

bool Server::acked(Globals::Header header, uint seq){
    if(header.ack == seq){
        return true;
    } else {
        if(seq < header.ack && header.ack - seq <= 32){
            return receivedPackets.test(header.ack - seq - 1);
        }
    }
    return false;
}

//the only issue i see is this relies on us receiving packets in order 
//to resend lost packets
void Server::handlePacket(Globals::Packet received){
    //rotate our received bitset by the difference between remoteSeqNum and receivedSeqNum
    //then set the bitset so the previous remoteSeqNum is acked

    //keep in mind packets may arrive out of order so sequence number may be less than our remote
    //sequence number
    if(received.header.seq > remoteSeqNum){
        receivedPackets = receivedPackets << received.header.seq - remoteSeqNum;
        //ack the previous remote seq num
        receivedPackets.set(received.header.seq - remoteSeqNum - 1);
        remoteSeqNum = received.header.seq;
    } else if(received.header.seq < remoteSeqNum && (remoteSeqNum - received.header.seq) < 32) {
        receivedPackets.set(remoteSeqNum - received.header.seq - 1);
    }

    //iterate over 
    for(auto it = sentPackets.cbegin(); it != sentPackets.cend(); ){
        Globals::PacketHandled packetHandled = it->second;
        //regardless of whether or not its been acked calculate time since sending message
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - packetHandled.sent).count();
        if(acked(received.header, it->first)){
            if(!rttDefined){
                rtt = elapsed;
                rttDefined = true;
            } else {
                rtt += Globals::rttSmooth * elapsed;
            }
            //if its been acked remove from list
            sentPackets.erase(it++);
            continue;
        } else {
            //if its been over resend time, resend the packet
            if(elapsed > Globals::packetLostTime){
                //The packet was lost
                packetHandled.onResend(packetHandled);

                //remove from list
                sentPackets.erase(it++);
                continue;
            }

        }
        ++it;
    }
}

void Server::sendInput(){
    fd_set fds;
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 0;
    int result = select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
    if(result && result != -1 && FD_ISSET(STDIN_FILENO, &fds)){
        //there is something to read
        std::getline(std::cin, message);
        std::cin.clear();
        if(message.length() && connectionEstablished){
            std::vector<Globals::AppData> data;
            data.push_back(Globals::AppData(currentMessage++, message));
            send(ip, clientPort, data);
            lastSentTime = std::chrono::high_resolution_clock::now();
        }
    }
}