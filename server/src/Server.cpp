#include "Server.hpp"

#include "Globals.hpp"
#include "Protocol.hpp"

#include <cstdio>
#include <iostream>

#if PLATFORM == PLATFORM_WINDOWS

#include <windows.h>
#define STDIN_FILENO _fileno(stdin)

void usleep(__int64 usec) {
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative
                                // value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

#else
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <random>

Server::Server() : socket(AF_INET) {
    socket.bind(Globals::port);
    socket.setNonBlocking();

// flush the input on windows so it doesn't wait for the next input
#if PLATFORM == PLATFORM_WINDOWS
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
}

void Server::update() {

    // TODO: add sleeps so we don't eat up cpu cycles
    // honestly create a thread for each connection and sleep the necessary
    // amount of time to send acks so if we're sending 30 times a second send
    // all the messages that have accumulated in that time
    for (auto it = connections.begin(); it != connections.end();) {
        // update all of our connections
        if (it->second->tryJoin()) {
            // the connection timed out
            delete it->second;
            it = connections.erase(it);
            continue;
        }
        it++;
    }

    // receive new messages either sending the message to respective
    // connection or establishing a new connection
    std::optional<MessageFrom> received = receive();
    if (received.has_value()) {
        std::string addr = received->from.toString();
        // process the received packet
        auto connection = connections.find(addr);
        if (connection != connections.end()) {
            connection->second->pushToReceive(received->packet);
        } else {
            Logger::logInfo("creating a new connection");
            // create a new connection
            const std::pair pair(addr, new Connection(socket));
            connections.insert(pair);
            auto &newConnection = connections.find(addr)->second;
            newConnection->connect(received->from);
            newConnection->start();
            newConnection->pushToReceive(received->packet);
        }
    }
}

std::optional<MessageFrom> Server::receive() {
    std::string received;
    auto from = socket.receiveFrom(received);
    if (from.has_value()) {
        // the message actually has content
        Protocol::Packet packet(received);

        // ensure the message is using our protocol
        if (std::string(packet.header.protocol) != Globals::protocol())
            return {};

        return (MessageFrom){(ConnectId){from->ip, from->port}, packet};
    }

    return {};
}
