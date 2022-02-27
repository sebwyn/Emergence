#include "Player.hpp"

#include <cstring>

void Player::loadBuffer(std::string &buffer){
    std::memcpy(&x, buffer.data(), sizeof(uint));
    std::memcpy(&y, buffer.data()+sizeof(uint), sizeof(uint));
}

std::string Player::toBuffer(){
    std::string buffer;
    buffer += std::string(sizeof(uint)*2, ' ');
    std::memcpy(buffer.data(), &x, sizeof(uint));
    std::memcpy(buffer.data()+sizeof(uint), &y, sizeof(uint));
    return buffer;
}
