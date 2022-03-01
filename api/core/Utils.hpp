#pragma once

#include <string>
#include <cstring>

typedef unsigned int uint;

class Utils {
public:
    static void writeUintToBuf(std::string &buffer, uint data) {
        uint offset = buffer.length();
        buffer += std::string(sizeof(uint), ' ');
        std::memcpy(buffer.data() + offset, &data, sizeof(uint));
    }

    static uint readUintFromBuf(const std::string &buffer, uint &offset) {
        uint out;
        std::memcpy(&out, buffer.data() + offset, sizeof(uint));
        offset += sizeof(uint);
        return out;
    }

    static std::string readStringFromBuf(const std::string &buffer, uint &offset, uint length){
        std::string out = std::string(buffer.data()+offset, length);
        offset += length;
        return out;
    }
private:

};
