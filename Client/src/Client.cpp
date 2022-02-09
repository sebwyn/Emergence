#include "Client.hpp"

#include "Globals.hpp"

#include <iostream>

Client::Client() : socket(AF_INET, SOCK_DGRAM) {
    socket.setNonBlocking();

    std::cout << "What ip would you like to connect to: ";
    std::cin >> ip;

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
            //set messages that have been acknowledged

        }

        //then send messages from queue
        std::getline(std::cin, message);
        send(message);
    }
}

void Client::send(std::string message){
    std::string packet = std::string(Globals::protocol()) + message;
    socket.sendTo(ip, Globals::port, packet);
}

std::optional<std::string> Client::receive(){
    std::string message;
    auto from = socket.receiveFrom(&message);
    if(from.has_value() && from->first == ip && from->second == Globals::port){
        return message;
    }
    return {};
}