#include "Game.hpp"

void Game::mainloop() {
    while (running) {

        std::vector<ConnectID> connections;
        server.getConnections(connections);

        for (auto connection : connections) {
            auto messages = server.getMessages(connection);
            std::optional<PlayerInfo> latestInfo = {}; 
            std::optional<PlayerData> latestData = {};
            Messenger::getLatest(messages, latestInfo, latestData);
            if(latestInfo.has_value()){
                if(players.find(connection.toString()) != players.end()){
                    Logger::logInfo("We're receiving the player info multiple times");
                } else {
                    players[connection.toString()] = Player();
                    players[connection.toString()].info = *latestInfo;
                }
            }
            if (latestData.has_value()) {
                Logger::logInfo("Got Player Position: " +
                                latestData->toString());
                if(players.find(connection.toString()) != players.end()){
                    players[connection.toString()].data = *latestData;
                } else {
                    Logger::logInfo("We never received the player info");
                }
            }
            server.flushMessages(connection);
        }

        // also need to read in all the meCossages from the clients
        // and update the world state based on that
        // the issue is that some packets may get dropped, so the server
        // needs to act as the authority, or we need to try and send packets
        // reliably
        world = World(width, height);
        for (auto player : players) {
            // add all the players to the world
            world.data[player.second.data.y][player.second.data.x] =
                player.second.info.symbol;
        }

        server.sendLatestMessage(Messenger::writeMessage(world));
        server.update();
    }
}
