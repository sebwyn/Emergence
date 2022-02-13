#pragma once

#include "Socket.hpp"

#include <chrono>
#include <vector>
#include <deque>

class Client {
public:
    Client();

    void update();
    void sendKeepAlive();
    void send(std::vector<Globals::AppData>& message);
    std::optional<Globals::Packet> receive();
private:
    Socket socket;
    std::string ip;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime, lastSentTime;
    bool connectionEstablished = false;
    uint remoteSeqNum = 0;
    uint localSeqNum = 0;

    uint currentMessage = 0;

    std::bitset<32> receivedPackets;

    std::deque<Globals::AppDataHandled> sentAppData;
};