#pragma once

#include <bitset>
#include <chrono>
#include <functional>
#include <map>
#include <string>

#include "Protocol.hpp"
#include "Socket.hpp"

typedef unsigned int uint;

struct ConnectID {
    std::string ip;
    ushort port;

    std::string toString() { return ip + ":" + std::to_string(port); }
};

struct MessageFrom {
    ConnectID from;
    Protocol::Packet packet;
};

enum Station { connecting, connected, disconnected };

class Connection {
  public:
    // by default a connection is not established, the first time
    // receive gets called on it it becomes established
    Connection(UdpSocket &socket);

    // updates station to either connected or disconnected
    // this can be read after a call and acted on appropriately
    void onConnect();
    void update();

    void connect(const std::string &_ip, ushort _port);
    void connect(const std::string &_ip, ushort _port, const std::string &message);

    // shouldn't be able to push a message unless connected
    // when we consider running these in a seperate thread mutexing is going
    // to get interesting
    void sendLatestMessage(const std::string &message);
    void sendMessage(const std::string &message);

    void sendKeepAlive();
    void send(
        std::vector<Protocol::AppData> &messages,
        std::function<void(Protocol::PacketHandled)> onResend =
            [](Protocol::PacketHandled) {});

    void receive(Protocol::Packet packet);

    std::string getIp() { return ip; }
    ushort getPort() { return port; }

    Station getStation() { return station; }

  private:
    // determine if a seq number is acked in a given packet
    bool acked(Protocol::Header header, uint seq);

    // set receivedPackets and localSeqNum based on an incoming packet
    void ack(Protocol::Packet packet);

    void updateMode();

    UdpSocket &socket;

    uint currentMessage = 0;
    std::string latestMessage;
    std::vector<std::string> messages;
    std::string connectionMessage;

    std::string ip;
    ushort port;

    Station station = disconnected;
    uint remoteSeqNum = 0;
    uint localSeqNum = 0;
    std::bitset<32> receivedPackets;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime,
        lastSentTime;

    std::map<uint, Protocol::PacketHandled> sentPackets;

    bool rttDefined;
    double rtt = 0.0;

    uint returnToGood = 15000;
    int trustedLevel = 0;
    Globals::ConnectionState mode = Globals::ConnectionState::GOOD;
    std::chrono::high_resolution_clock::time_point modeStart, lastTrustTime;
};
