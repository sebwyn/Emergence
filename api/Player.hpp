#pragma once

#include <string>

struct Player {
    int x, y;

    //server-side functions
    void loadBuffer(std::string &buffer);

    //client-side functions
    std::string toBuffer();
};
