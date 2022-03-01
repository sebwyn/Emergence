// needs to render and run game, pulling in data from the client
// and pushing move info to the server
#pragma once

#include "Client.hpp"
#include "Player.hpp"
#include "World.hpp"

class Game {
  public:
    Game(){ startCurses(); }
    ~Game(){ killCurses(); }

    void mainloop();

  private:

    void startCurses();
    void defaultCurses();
    void customCurses();
    void killCurses();

    bool connectionSequence(uint line);
    void handleInput();

    PlayerInfo playerInfo = PlayerInfo("@");
    PlayerData player = {0, 0};
    World world;

    Client client;
    bool running = true;
};
