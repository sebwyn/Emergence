#include "Game.hpp"

void Game::mainloop(){
    while(running){
        server.sendLatestMessage(world.toBuffer());
        server.update();

        //also need to read in all the messages from the clients
        //and update the world state based on that
        //the issue is that some packets may get dropped, so the server needs
        //to act as the authority, or we need to try and send packets reliably
    }
}


