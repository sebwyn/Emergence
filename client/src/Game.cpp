#include "Game.hpp"

#include <ncurses.h>

#include "Messenger.hpp"

using namespace std::literals::chrono_literals;

void Game::mainloop() {

    while (running) {

        if (client.getStation() == connected) {

            handleInput();

            // push the player position as a message to the server
            client.sendMessage(Messenger::writeMessage(player));

            // get the most up to date world and draw
            std::vector<Protocol::AppData> messages = client.getMessages();
            if (messages.size() > 0) {
                std::optional<World> latestWorld = {};
                Messenger::getLatest(messages, latestWorld);
                if (latestWorld.has_value()) {
                    //Logger::logInfo("Got updated world data");
                    world = *latestWorld;
                }
            }
            client.flushMessages();

            if (world.data) {
                world.draw();
            }
            refresh();

            client.update();

            std::this_thread::sleep_for(5ms);
        } else {
            // display reconnect screen
            erase();
            defaultCurses();
            if (!connectionSequence(0)) {
                running = false;
            }
            erase();
            customCurses();
        }
    }

    client.disconnect();
    client.join();
}

void Game::startCurses() { initscr(); }

void Game::customCurses() {
    cbreak();
    noecho();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    curs_set(0);
}

void Game::defaultCurses() {
    nocbreak();
    echo();
    keypad(stdscr, false);
    nodelay(stdscr, false);
    curs_set(1);
}

void Game::killCurses() { endwin(); }

bool Game::connectionSequence(uint line) {
    mvaddstr(line, 0, "Ip you would like to connect to: ");
    char buf[100];
    getstr(buf);
    line++;

    mvaddstr(line, 0, "Connecting.  ");
    std::string ip(buf);

    if (ip == "quit") {
        return false;
    }

    client.connect(ip, Messenger::writeMessage(playerInfo));
    uint numPings = 0;
    auto lastPing = std::chrono::high_resolution_clock::now();
    while (client.getStation() == connecting) {
        // print connecting animation with 3 dots
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                  lastPing)
                .count() > 1000) {
            numPings++;
            lastPing = now;
            if (numPings % 3 == 0)
                mvaddstr(line, 0, "Connecting.  ");
            else if (numPings % 3 == 1)
                mvaddstr(line, 0, "Connecting.. ");
            else if (numPings % 3 == 2)
                mvaddstr(line, 0, "Connecting...");

            refresh();
        }

        client.update();
    }
    line++;
    if (client.getStation() == disconnected) {
        addstr(" Failed to connect!");
        return connectionSequence(line);
    }

    return true;
}

// TODO: collision checking on the server and send back position data to the
// client
void Game::handleInput() {
    int ch = getch();
    switch (ch) {
    case ERR:
        break;
    case 'q':
        running = false;
        break;
    case 'w':
        player.y--;
        if (player.y < 0)
            player.y = 0;
        break;
    case 'a':
        player.x--;
        if (player.x < 0)
            player.x = 0;
        break;
    case 's':
        player.y++;
        if (player.y >= world.height)
            player.y = world.height - 1;
        break;
    case 'd':
        player.x++;
        if (player.x >= world.width)
            player.x = world.width - 1;
        break;
    }
}
