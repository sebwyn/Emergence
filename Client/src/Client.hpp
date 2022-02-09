#pragma once

#include "Socket.hpp"

#include <chrono>

class Client {
public:
    Client();

    void update();
    void send(std::string message);
    std::optional<std::string> receive();
private:
    Socket socket;
    std::string ip;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime;
    bool connectionEstablished = false;
};