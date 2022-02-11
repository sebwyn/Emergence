#include "Server.hpp"

#include "Globals.hpp"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

Server::Server() : socket(AF_INET, SOCK_DGRAM) {
    socket.bind(Globals::port);
    socket.setNonBlocking();

    receivedPackets.set();
}

void Server::update(){
    //receive messages and ack
    ackTime = std::chrono::high_resolution_clock::now();
    while(true){
        if(connectionEstablished){
            auto now = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastPacketTime).count() > 10){
                std::cout << "The connection timed out! Keeping the server alive" << std::endl;
            }

            //send a message to the client every second that keeps the connection alive, and acks
            if(std::chrono::duration_cast<std::chrono::seconds>(now - ackTime).count() > 1){
                send(ip, "ack");
                ackTime = now;
            }
        }

        //receive a valid packet per our protocol, and establish a connection if 
        //one is not already established
        auto received = receive();
        if(received.has_value()){
            lastPacketTime = std::chrono::high_resolution_clock::now();
            //set messages that have been acknowledged
            //ensure the message has a full header
            Globals::Header header;
            memcpy(&header, received->c_str(), sizeof(Globals::Header));
            //update the bitset of requests
            receivedPackets << header.seq - remoteSeqNum;
            receivedPackets.set(header.seq - remoteSeqNum);
            remoteSeqNum = header.seq;

            std::string message(*received);
            message = message.substr(sizeof(Globals::Header), sizeof(message)-sizeof(Globals::Header));
            std::cout << "Mesg: " << message << std::endl;
        }
    }
}

std::optional<std::string> Server::receive(){
    std::string message;
    auto received = socket.receiveFrom(&message);
    if(received.has_value()){
        //the message actually has content
        //check if 
        if(!connectionEstablished){
            //ensure this packet is valid per our protocol, if it is establish connection and save ip
            if(message.length() < sizeof(Globals::Header)){
                //drop the packet
                return {};
            }
            connectionEstablished = true;
            ip = received->first;
        } else if(received->first != ip){
            //drop the packet if it isn't coming from the one client
            //TODO: make it so another connection is created
            return {};
        }

        //ensure the message is using our protocol
        if(message.length() <= 4 || message.substr(0, 4) != Globals::protocol())
            return {};

        return message;
    }
    return {};
}

void Server::send(std::string ip, std::string message){
    Globals::Header header;
    header.seq = localSeqNum++;
    header.ack = remoteSeqNum;
    header.acks = receivedPackets;

    std::string toSend = std::string(sizeof(header), ' ');
    memcpy(toSend.data(), &header, sizeof(header));
    toSend += message;
    std::cout << "sending to " << ip << ": " << toSend << std::endl;
    socket.sendTo(ip, Globals::port, toSend);
}