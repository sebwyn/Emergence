#include "Client.hpp"

#include "Globals.hpp"

#include <iostream>

Client::Client() : socket(AF_INET, SOCK_DGRAM) {
    socket.setNonBlocking();

    std::cout << "What ip would you like to connect to: ";
    std::cin >> ip;

    //set receivedPackets to high
    receivedPackets.set();

    //socket.connect(ip, Globals::port);
    //std::cout << "Connected" << std::endl;
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
        }

        //receive messages first (so we know what has been acknowledged)
        auto received = receive();
        if(received.has_value()){
            if(!connectionEstablished) connectionEstablished = true;
            lastPacketTime = std::chrono::high_resolution_clock::now();

            if(received->length() < sizeof(Globals::Header)){
                //drop the packet
                continue;
            }
            Globals::Header header;
            memcpy(&header, received->c_str(), sizeof(header));
            
            //rotate our received bitset by the difference between remoteSeqNum and receivedSeqNum
            //then set the bitset so the previous remoteSeqNum is acked
            receivedPackets << header.seq - remoteSeqNum;
            receivedPackets.set(header.seq - remoteSeqNum);
            remoteSeqNum = header.seq;

            std::cout << "Received ack: " << header.ack << " " << header.acks.to_string() << std::endl;
        }

        //then send messages from queue
        std::getline(std::cin, message);
        send(message);
    }
}

void Client::send(std::string message){
    Globals::Header header;
    header.seq = localSeqNum++;
    header.ack = remoteSeqNum;
    header.acks = receivedPackets;

    std::string toSend = std::string(sizeof(header), ' ');
    memcpy(toSend.data(), &header, sizeof(header));
    toSend += message;
    std::cout << "sending: " << toSend << std::endl;
    socket.sendTo(ip, Globals::port, toSend);
}

std::optional<std::string> Client::receive(){
    std::string message;
    auto from = socket.receiveFrom(&message);
    if(from.has_value() && from->first == ip && from->second == Globals::port){
        return message;
    }
    return {};
}