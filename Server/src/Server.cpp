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
    std::string message;
    while(message != "quit"){
        auto updateStart = std::chrono::high_resolution_clock::now();

        if(connectionEstablished){
            if(std::chrono::duration_cast<std::chrono::seconds>(updateStart - lastPacketTime).count() > 10){
                std::cout << "The connection timed out!" << std::endl;
                break;
            }

            //send a message to the client every second that keeps the connection alive, and acks
            if(std::chrono::duration_cast<std::chrono::milliseconds>(updateStart - lastSentTime).count() > 500){
                sendKeepAlive(ip, clientPort);
                lastSentTime = updateStart;
            }

            resendPackets();
        }

        //receive a valid packet per our protocol, and establish a connection if 
        //one is not already established
        auto received = receive();
        if(received.has_value()){
            lastPacketTime = std::chrono::high_resolution_clock::now();

            updateSentPackets(*received);

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
            //std::cout << "Dropping packet" << std::endl;
            return {};
        }

        return packet;
    }
    return {};
}

void Server::sendKeepAlive(std::string ip, ushort port){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header);

    socket.sendTo(ip, port, packet.toBuffer());
}

void Server::send(std::string ip, int port, std::vector<Globals::AppData>& messages){
    Globals::Header header(localSeqNum, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header, messages);

    //add messages to sent messages
    for(Globals::AppData message : messages){
        sentAppData.push_back(Globals::AppDataHandled(localSeqNum, message, [](Globals::AppData){}));
    }
    localSeqNum++;

    socket.sendTo(ip, port, packet.toBuffer());
}

void Server::resendPackets(){
    //iterate over all the sent packets, checking if any have been lost and resending
    auto i = sentAppData.begin();
    for(Globals::AppDataHandled packet : sentAppData){
        //if the packet is still in this list after a second, resend it  
        auto now = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - packet.sent).count() > Globals::packetLostTime){
            std::cout << "Resending packet: " << packet.data.toString() << " after: " << std::chrono::duration_cast<std::chrono::milliseconds>(now - packet.sent).count() << std::endl;
            packet.onResend(packet.data);
            packet.resent = true;
            //remove from the list
            if(std::next(i) == sentAppData.cend()){
                sentAppData.erase(i);
                break;
            }
            sentAppData.erase(i);
        }  
        ++i; 
    }
}

void Server::updateSentPackets(Globals::Packet received){

    //set messages that have been acknowledged
    //update the bitset of requests
    if(received.header.seq > remoteSeqNum){
        receivedPackets = receivedPackets << received.header.seq - remoteSeqNum;
        //ack the previous remote seq num
        receivedPackets.set(received.header.seq - remoteSeqNum - 1);
        remoteSeqNum = received.header.seq;
    } else if(received.header.seq < remoteSeqNum && (remoteSeqNum - received.header.seq) < 32) {
        receivedPackets.set(remoteSeqNum - received.header.seq - 1);
    }

    auto i = sentAppData.cbegin();
    while(i != sentAppData.cend()){
        if(received.header.ack == i->seq){
            //remove from the list, problematic if the last element
            if(std::next(i) == sentAppData.cend()){
                sentAppData.erase(i);
                break;
            }
            sentAppData.erase(i);
        }

        if(i->seq < received.header.ack){
            if(i->seq - received.header.ack <= 32){
                if(received.header.acks.test(i->seq - received.header.ack - 1)){
                    if(std::next(i) == sentAppData.cend()){
                        sentAppData.erase(i);
                        break;
                    }
                    sentAppData.erase(i);
                } else {
                    std::cout << "Damn that wasn't received!" << std::endl;
                }
            } else {
                if(!i->resent){
                    auto now = std::chrono::high_resolution_clock::now();
                    std::cout << "Seriously not acked after 32 packets? and " << std::chrono::duration_cast<std::chrono::milliseconds>(now - i->sent).count() << " ms and " << i->seq << " " << remoteSeqNum << std::endl;
                    if(std::next(i) == sentAppData.cend()){
                        sentAppData.erase(i);
                        break;
                    }
                    sentAppData.erase(i);
                } else {
                    std::cout << "Not acked after 32 packets and not resent. That's whack!" << std::endl;
                }
            }
        }
        ++i;
    }
}

void Server::sendInput(){
    std::string message;
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