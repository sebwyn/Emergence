// needs to render and run game, pulling in data from the client
// and pushing move info to the server
#pragma once

#include <ncurses.h>

#include "Client.hpp"
#include "GameStructures.hpp"

class Game {
  public:
    Game();
    ~Game();

    void mainloop();

  private:

    PlayerInfo playerInfo = PlayerInfo("@");
    PlayerData player = {0, 0};
    World world;

    void startCurses();
    void defaultCurses();
    void customCurses();
    void killCurses();

    bool connectionSequence(uint line);

    Client client;
    bool running = true;
};
