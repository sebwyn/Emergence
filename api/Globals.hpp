#pragma once

#include <bitset>
#include <sstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <functional>
#include <memory>
#include <cstring>

typedef unsigned int uint;
typedef unsigned short ushort;

class Globals {
public:
    static const int maxMsgLength = 1024;

    static const int port = 1337;
    //time before packet resend is called when packet hasn't been acked in ms
    static const int packetLostTime = 1000;
    //float percentage of packet being lost
    static constexpr double packetLoss = .02;
    static std::string protocol(){
        return std::string("MERG");
    }
    static constexpr int fps = 60;

    //time before connection times out in seconds
    static constexpr int timeout = 10;
    
    //for congestion avoidance
    static constexpr float rttSmooth = 0.1f;
    static constexpr int goodSendRate = 30;
    static constexpr int badSendRate = 10;

    static constexpr int badRtt = 250;
    
    enum ConnectionState {
        GOOD,
        BAD,
    };
};