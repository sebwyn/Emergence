#pragma once

#include "Connection.hpp"
#include "Protocol.hpp"
#include "Socket.hpp"

class Server {
  public:
    Server();

    void update();

    void getConnections(std::vector<ConnectId> &c) {
        for (auto it = connections.begin(); it != connections.end(); it++) {
            c.push_back(ConnectId(it->first));
        }
    }

    // push a message and then once every update time send the latest message
    // from the server to all clients
    void sendMessage(const std::string &buffer) {
        for (auto it = connections.begin(); it != connections.end(); it++) {
            it->second.sendMessage(buffer);
        }
    }
    void sendLatestMessage(const std::string &buffer) {
        for (auto it = connections.begin(); it != connections.end(); it++) {
            it->second.sendLatestMessage(buffer);
        }
    }

    // TODO: call these directly from getting connections
    std::vector<Protocol::AppData> &getMessages(ConnectId connection) {
        return connections.at(connection.toString()).getMessages();
    }
    void flushMessages(ConnectId connection) {
        connections.at(connection.toString()).flushMessages();
    }

  private:
    std::optional<MessageFrom> receive();

    // functions called once; defined to cleanup update loop
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
