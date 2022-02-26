#include "Game.hpp"

Game::Game() { startCurses(); }

Game::~Game() {}

void Game::mainloop() {

    addstr("Hello World!");

    while (running) {
        // get input and send it to the server
        int ch = getch();
        switch (ch) {
        case ERR:
            break;
        case 'Q':
            running = false;
            break;
        }

        // update the ncurses display based on packets received from the server
        // with the world data
        refresh();

        // manage
        /*if(client->getConnected()){
            client->update();
        } else {
            //kill ncurses and prompt for reconnection

        }*/
    }
}

void Game::startCurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    nodelay(stdscr, true);
}
