#pragma once

#include "GameStructures.hpp"
#include "Server.hpp"

class Game {
  public:
    Game() : world(width, height) {}

    void mainloop();

  private:
    struct Player {
        PlayerInfo info;
        PlayerData data;

        Player() : info('@'), data(0, 0) {}
    };

    const uint width = 10;
    const uint height = 10; 

    World world;
    Server server;

    std::map<std::string, Player> players;
    bool running = true;
};