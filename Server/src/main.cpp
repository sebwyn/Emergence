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
    Messenger::getLatest(messages, *playerData);
    std::cout << Messenger::writeMessage(data) << std::endl;
    std::cout << playerData->x << std::endl;
    */

    return 0;
}
