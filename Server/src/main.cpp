#include "Game.hpp"

#include "GameStructures.hpp"
#include "Protocol.hpp"

int main() {
    
    Logger::create("serverLog.txt", "Emergence/");
    Logger::logInfo("Starting server");

    Game game;
    game.mainloop();
    
    /*
    PlayerData data(0, 0);
    Protocol::AppData message(0, Messenger::writeMessage(data));

    std::vector<Protocol::AppData> messages = {
        message,
    };

    std::optional<PlayerData> playerData = {};
    Messenger::getLatest(messages, playerData);
    if(playerData.has_value()){
        std::cout << playerData->toString() << std::endl;
    }
    */

    return 0;
}
