#pragma once

#include <bitset>

class Globals {
public:
    static const int port = 1337;
    static const char *protocol(){
        return "MERG";
    }

    struct Header {
        char protocol[4] = {'M', 'E', 'R', 'G'};
        unsigned int seq;
        uint ack;
        std::bitset<32> acks;

    };
};