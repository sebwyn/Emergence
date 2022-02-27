#include "Socket.hpp"

#include "Server.hpp"

int main(){

    Logger::create("serverLog.txt");
    Logger::logInfo("Starting server");

    Server server;
    server.update();

    return 0;
}
