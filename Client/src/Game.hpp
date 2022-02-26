//needs to render and run game, pulling in data from the client
//and pushing move info to the server
#pragma once

#include <ncurses.h>

#include "Client.hpp"

class Game {
public:
    Game();
    ~Game();

    void mainloop();
private:
    
    void startCurses();
    void killCurses();

    std::unique_ptr<Client> client;

    bool running = true;
};
