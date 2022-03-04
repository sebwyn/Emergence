#include "Game.hpp"

int main(){

    Logger::create("clientLog.txt", "Emergence/");
    Logger::logInfo("Starting client");
    
    Game game;
    game.mainloop();

    /*
    Client client;
    client.connect("127.0.0.1");

    while(true){
        client.update();

        for(auto message : client.getMessages()){
            std::cout << "Received Message: " + message.toString() << std::endl;
        }
        client.flushMessages();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    */

    return 0;
}
