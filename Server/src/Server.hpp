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
    
    //push a message and then once every update time send the latest message from the server to 
    //all clients
    void sendMessage(const std::string& buffer);
    void sendLatestMessage(const std::string& buffer);
private:
    std::optional<MessageFrom> receive();

    //functions called once; defined to cleanup update loop
    void handlePacket(Protocol::Packet received);
    void sendInput();
    void manageMode();
    
    std::vector<std::string> messsages;

    UdpSocket socket;
    std::map<std::string, Connection> connections;

    uint currentMessage = 0;

    uint returnToGood = 15000;
    int trustedLevel = 0;
    Globals::ConnectionState mode = Globals::ConnectionState::GOOD;
    std::chrono::high_resolution_clock::time_point modeStart, lastTrustTime;
};
