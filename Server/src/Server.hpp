#pragma once

#include "Socket.hpp"

class Server {
public:
    Server();

    void update();

    //receive a message filtering with our protocol
    std::optional<std::string> receive();
    void send(std::string ip, std::string message);
private:
    Socket socket;

    //ip of the single client
    std::string ip;

    bool connectionEstablished = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime, ackTime;
    uint remoteSeqNum = 0;

    uint localSeqNum = 0;

    std::bitset<32> receivedPackets;
};