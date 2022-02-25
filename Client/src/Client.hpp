#pragma once

#include "Socket.hpp"

#include <chrono>
#include <vector>
#include <deque>
#include <map>

class Client {
public:
    Client();

    void update();
    void sendKeepAlive();
    void send(std::vector<Globals::AppData>& message, std::function<void(Globals::PacketHandled)> onResend = [](Globals::PacketHandled){});
    std::optional<Globals::Packet> receive();
private:

    //determine if sequence number is acked given a header
    bool acked(Globals::Header header, uint seq);

    //functions called once; defined to cleanup update loop
    void manageMode();
    void handlePacket(Globals::Packet received);
    void sendInput();

    std::string message = "";

    UdpSocket socket;
    std::string ip;
    bool connectionEstablished = false;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime, lastSentTime;
    uint remoteSeqNum = 0;
    uint localSeqNum = 0;
    std::bitset<32> receivedPackets;

    uint currentMessage = 0;
    std::map<uint, Globals::PacketHandled> sentPackets;

    bool rttDefined = false;
    double rtt;

    uint returnToGood = 15000;
    int trustedLevel = 0;
    Globals::ConnectionState mode = Globals::ConnectionState::GOOD;
    std::chrono::high_resolution_clock::time_point modeStart, lastTrustTime;

};