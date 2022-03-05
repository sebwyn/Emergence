#include "Game.hpp"

#include "Messenger.hpp"

using namespace std::literals::chrono_literals;

void Game::mainloop() {
    while (running) {

        std::vector<ConnectId> connections;
        server.getConnections(connections);

        for (auto connection : connections) {
            auto messages = server.getMessages(connection);
            std::optional<PlayerInfo> latestInfo = {};
            std::optional<PlayerData> latestData = {};
            Messenger::getLatest(messages, latestInfo, latestData);
            if (latestInfo.has_value()) {
                if (players.find(connection.toString()) != players.end()) {
                    Logger::logInfo(
                        "We're receiving the player info multiple times");
                } else {
                    players[connection.toString()] = Player();
                    players[connection.toString()].info = *latestInfo;
                }
            }
            if (latestData.has_value()) {
                if (players.find(connection.toString()) != players.end()) {
                    players[connection.toString()].data = *latestData;
                } else {
                    Logger::logInfo("We never received the player info");
                }
            }
            server.flushMessages(connection);
        }

        world = World(width, height);
        for (auto player : players) {
            // add all the players to the world
            world.data[player.second.data.y][player.second.data.x] =
                player.second.info.symbol;
        }

        server.sendMessage(Messenger::writeMessage(world));
        //Logger::logInfo(Messenger::writeMessage(world));

        server.update();

        std::this_thread::sleep_for(10ms);
    }
}
