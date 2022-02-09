#pragma once

#include "Socket.hpp"

class Server {
public:
    Server();

    void update();

    //receive a message filtering with our protocol
    std::optional<std::string> receive();
private:
    Socket socket;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime;
    bool connectionEstablished = false;
};