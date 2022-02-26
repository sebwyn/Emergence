#pragma once

#include "Socket.hpp"
#include "Protocol.hpp"
#include "Connection.hpp"

#include <chrono>
#include <vector>
#include <deque>
#include <map>

class Client {
public:
    Client();

    void connect(std::string ip);

    void update();

    void sendKeepAlive();
    void send(std::vector<Protocol::AppData>& message, std::function<void(Protocol::PacketHandled)> onResend = [](Protocol::PacketHandled){});
    std::optional<MessageFrom> receive();

    bool getConnected(){ return connected; }
private:

    //functions called once; defined to cleanup update loop
    void sendInput();

    std::string message = "";

    UdpSocket socket;
    std::unique_ptr<Connection> connection;

    uint currentMessage = 0;

    bool connected = false;
};
