#include "Game.hpp"

#include <iostream>

int main(){

    Logger::create("clientLog.txt", "Emergence/");
    Logger::logInfo("Starting client");
    
    Game game;
    game.mainloop();

    return 0;
}