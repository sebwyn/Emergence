#pragma once

#include "Socket.hpp"

#include <deque>

class Server {
public:
    Server();

    void update();

    //receive a message filtering with our protocol
    std::optional<Globals::Packet> receive();
    void sendKeepAlive(std::string ip, ushort port);
    void send(std::string ip, int port, std::vector<Globals::AppData>& message);
private:
    Socket socket;

    //ip of the single client
    std::string ip;
    int clientPort;

    bool connectionEstablished = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime, lastSentTime;
    uint remoteSeqNum = 0;
    uint localSeqNum = 0;

    std::bitset<32> receivedPackets;

    std::deque<Globals::AppDataHandled> sentAppData;
    uint currentMessage = 0;
};