#pragma once

#include "Socket.hpp"
#include "Protocol.hpp"
#include "Connection.hpp"

#include <deque>
#include <map>

class Server {
public:
    Server();

    void update();
private:
    std::optional<MessageFrom> receive();

    //functions called once; defined to cleanup update loop
    void handlePacket(Protocol::Packet received);
    void sendInput();
    void manageMode();

    std::string message = "";

    UdpSocket socket;
    std::map<std::string, Connection> connections;

    uint currentMessage = 0;

    uint returnToGood = 15000;
    int trustedLevel = 0;
    Globals::ConnectionState mode = Globals::ConnectionState::GOOD;
    std::chrono::high_resolution_clock::time_point modeStart, lastTrustTime;
};
