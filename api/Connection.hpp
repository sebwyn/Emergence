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

    std::string toString(){
        return ip + ":" + std::to_string(port);
    }
};

struct MessageFrom {
    ConnectID from;
    Protocol::Packet packet;
};

class Connection {
  public:
    //by default a connection is not established, the first time 
    //receive gets called on it it becomes established
    Connection(UdpSocket &socket, std::string ip, ushort port);

    // returns whether the connection should stay alive
    bool update();

    void sendKeepAlive();
    void send(
        std::vector<Protocol::AppData> &messages,
        std::function<void(Protocol::PacketHandled)> onResend =
            [](Protocol::PacketHandled) {});

    void receive(Protocol::Packet packet);

    std::string getIp() { return ip; }
    ushort getPort() { return port; }

  private:
    
    //determine if a seq number is acked in a given packet
    bool acked(Protocol::Header header, uint seq);

    //set receivedPackets and localSeqNum based on an incoming packet
    void ack(Protocol::Packet packet);

    void updateMode();

    UdpSocket &socket;

    std::string ip;
    ushort port;

    bool established = false;

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
