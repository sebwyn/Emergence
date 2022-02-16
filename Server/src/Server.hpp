#pragma once

#include "Socket.hpp"

#include <deque>
#include <map>

class Server {
public:
    Server();

    void update();

    /* need to decide if pushing and popping messages is the best format for sending messages
    void pushMessage(Globals::AppData message, std::function<void(Globals::AppData)> onResend = 
        [](Globals::AppData data) {});
    
    Globals::AppData popMessage();
    */

private:

    //determine if sequence number is acked given a header
    bool acked(Globals::Header header, uint seq);

    //functions called once; defined to cleanup update loop
    void handlePacket(Globals::Packet received);
    void sendInput();
    void manageMode();

    //receive a message filtering with our protocol
    std::optional<Globals::Packet> receive();
    void sendKeepAlive(std::string ip, ushort port);
    void send(std::string ip, int port, std::vector<Globals::AppData>& message, std::function<void(Globals::PacketHandled)> onResend = [](Globals::PacketHandled){});

    std::string message = "";

    Socket socket;

    //ip of the single client
    std::string ip;
    int clientPort;

    bool connectionEstablished = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime, lastSentTime;
    uint remoteSeqNum = 0;
    uint localSeqNum = 0;

    std::bitset<32> receivedPackets;

    std::map<uint, Globals::PacketHandled> sentPackets;
    uint currentMessage = 0;

    bool rttDefined;
    double rtt = 0.0;

    uint returnToGood = 15000;
    int trustedLevel = 0;
    Globals::ConnectionState mode = Globals::ConnectionState::GOOD;
    std::chrono::high_resolution_clock::time_point modeStart, lastTrustTime;
};