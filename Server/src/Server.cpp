#include "Server.hpp"

#include "Globals.hpp"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

Server::Server() : socket(AF_INET, SOCK_DGRAM) {
    socket.bind(Globals::port);
    socket.setNonBlocking();
}

void Server::update(){
    //receive messages and ack
    while(true){
        if(connectionEstablished){
            auto now = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastPacketTime).count() > 10){
                std::cout << "The connection timed out! Keeping the server alive" << std::endl;
            }
        }

        //receive messages first (so we know what has been acknowledged)
        auto received = receive();
        if(received.has_value()){
            if(!connectionEstablished) connectionEstablished = true;
            lastPacketTime = std::chrono::high_resolution_clock::now();
            //set messages that have been acknowledged

        }
    }
}

std::optional<std::string> Server::receive(){
    std::string message;
    auto received = socket.receiveFrom(&message);
    if(received.has_value()){
        //the message actually has content

        //ensure the message is using our protocol
        if(message.length() <= 4 || message.substr(0, 4) != Globals::protocol())
            return {};

        message = message.substr(4, message.length()-4);
        return message;
    }
    return {};
}