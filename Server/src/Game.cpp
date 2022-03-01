#include "Game.hpp"

void Game::mainloop() {
    while (running) {

        std::vector<ConnectID> connections;
        server.getConnections(connections);

        // Logger::logInfo("Got " + std::to_string(connections.size()) + "
        // connections");
        for (auto connection : connections) {
            auto messages = server.getMessages(connection);
            for (auto message : messages) {
                Logger::logInfo(message.toString());
            }
            std::optional<PlayerData> latestData = {};
            Messenger::getLatest(messages, *latestData);
            auto msg = Messenger::readMessage<PlayerData>(messages[0].message);
            if (latestData.has_value()) {
                std::cout
                    << "Got Player Position: " << latestData->toString()
                    << std::endl;
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

            server.sendLatestMessage(world.toBuffer());
            server.update();
        }
    }
