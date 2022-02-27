#pragma once

#include "Server.hpp"
#include "World.hpp"
#include "Player.hpp"

class Game {
public:
    void mainloop();
private:
    World world;
    Server server;

    std::vector<Player> players;
    bool running = true;
};
