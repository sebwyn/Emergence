#include "Game.hpp"

int main() {
    
    Logger::create("serverLog.txt", "Emergence/");
    Logger::logInfo("Starting server");

    Game game;
    game.mainloop();
    
    return 0;
}
