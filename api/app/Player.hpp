#pragma once

#include "Utils.hpp"

typedef unsigned int uint;

struct PlayerInfo {
    static const uint id = 0;

    char symbol;
    PlayerInfo(char symbol) : symbol(symbol) {}
    PlayerInfo(const std::string &buffer) { symbol = buffer[0]; }

    std::string toBuffer() const { return std::string(1, symbol); }
    std::string toString() const {
        return "(PlayerInfo: (symbol: " + std::string(1, symbol) + "))";
    }
};

struct PlayerData {
    static const uint id = 1;

    int x, y;
    PlayerData(int x, int y) : x(x), y(y) {}
    PlayerData(const std::string &buffer) {
        uint offset = 0;
        x = Utils::readUintFromBuf(buffer, offset);
        y = Utils::readUintFromBuf(buffer, offset);
    }

    std::string toBuffer() const {
        std::string buffer;
        Utils::writeUintToBuf(buffer, x);
        Utils::writeUintToBuf(buffer, y);
        return buffer;
    }
    std::string toString() const {
        return "(PlayerData: (x: " + std::to_string(x) +
               ", y: " + std::to_string(y) + "))";
    }
};

