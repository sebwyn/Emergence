#include "Client.hpp"

#include "Globals.hpp"

#include <iostream>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>

Client::Client() : socket(AF_INET, SOCK_DGRAM) {
    socket.bind(Globals::port+1);
    socket.setNonBlocking();

    std::cout << "What ip would you like to connect to: ";
    std::cin >> ip;

    //set receivedPackets to high
    receivedPackets.set();
}

void Client::update(){
    std::string message;
    while(message != "quit"){
        //check to see if the connection has timed out
        
        if(connectionEstablished){
            auto now = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastPacketTime).count() > 10){
                std::cout << "The connection timed out!" << std::endl;
                break;
            }

            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastSentTime).count() > 1){
                //send a keep alive
                sendKeepAlive();
                lastSentTime = std::chrono::high_resolution_clock::now();
            }
        }

        //receive messages first (so we know what has been acknowledged)
        auto received = receive();
        if(received.has_value()){
            if(!connectionEstablished)
                connectionEstablished = true;
        
            lastPacketTime = std::chrono::high_resolution_clock::now();
            
            //rotate our received bitset by the difference between remoteSeqNum and receivedSeqNum
            //then set the bitset so the previous remoteSeqNum is acked

            //keep in mind packets may arrive out of order so sequence number may be less than our remote
            //sequence number
            if(received->header.seq > remoteSeqNum){
                receivedPackets << received->header.seq - remoteSeqNum;
                //ack the previous remote seq num
                receivedPackets.set(received->header.seq - remoteSeqNum - 1);
                remoteSeqNum = received->header.seq;
            } else if(received->header.seq < remoteSeqNum && (remoteSeqNum - received->header.seq) < 32) {
                receivedPackets.set(remoteSeqNum - received->header.seq - 1);
            }

            //update sentPackets, acking anything that has been acked
            auto i = sentAppData.cbegin();
            while(i != sentAppData.cend()){
                if(received->header.ack == i->seq){
                    //remove from the list, problematic if the last element
                    if(std::next(i) == sentAppData.cend()){
                        sentAppData.erase(i);
                        break;
                    }
                    sentAppData.erase(i);
                }

                if(i->seq < remoteSeqNum){
                    if(i->seq - remoteSeqNum <= 32){
                        if(std::next(i) == sentAppData.cend()){
                        sentAppData.erase(i);
                        break;
                    }
                    sentAppData.erase(i);
                    } else {
                        std::cout << "Seriously not acked after 32 packets?" << std::endl;
                    }
                }
                ++i;
            }
            
            //not a keep alive
            if(received->messageCount){
                for(Globals::AppData message : received->messages){
                    std::cout << "H:" << message.toString() << std::endl;
                }
            }
        }

        //iterate over all the sent packets, checking if any have been lost and resending
        auto i = sentAppData.begin();
        for(Globals::AppDataHandled packet : sentAppData){
            //if the packet is still in this list after a second, resend it  
            auto now = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration_cast<std::chrono::milliseconds>(now - packet.sent).count() > Globals::packetLostTime){
                std::cout << "Resending packet: " << packet.data.toString() << std::endl;
                packet.onResend(packet.data);
                //remove from the list
                if(std::next(i) == sentAppData.cend()){
                    sentAppData.erase(i);
                    break;
                }
                sentAppData.erase(i);
            }  
            ++i; 
        }

        //then send messages from queue
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
            if(message.length()){
                std::vector<Globals::AppData> data;
                data.push_back(Globals::AppData(currentMessage++, message));
                send(data);
                lastSentTime = std::chrono::high_resolution_clock::now();
            }
        }

    }
}

void Client::sendKeepAlive(){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header);

    socket.sendTo(ip, Globals::port, packet.toBuffer());
}

void Client::send(std::vector<Globals::AppData>& messages){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header, messages);

    socket.sendTo(ip, Globals::port, packet.toBuffer());
}

std::optional<Globals::Packet> Client::receive(){
    std::unique_ptr<char> received;
    auto from = socket.receiveFrom(&received);
    if(from.has_value() && from->first == ip && from->second == Globals::port){
        Globals::Packet packet(received);

        //ensure protocol
        if(std::string(packet.header.protocol) != Globals::protocol())
            return {};

        return packet;
    }
    return {};
}