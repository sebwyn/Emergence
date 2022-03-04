#include "Game.hpp"

int main() {
    
    Logger::create("serverLog.txt", "Emergence/");
    Logger::logInfo("Starting server");

    Game game;
    game.mainloop();
    
    /*
    Server server;
    
    while(true){
        server.update();

        server.sendMessage("Hello there, welcome!");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    */
    return 0;
}
