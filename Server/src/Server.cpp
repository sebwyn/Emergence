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

    //TODO: add sleeps so we don't eat up cpu cycles
    //honestly create a thread for each connection and sleep the necessary amount of time to send acks
    //so if we're sending 30 times a second send all the messages that have accumulated in that time
    while (true) {
        for (auto it = connections.begin(); it != connections.end();) {
            // update all of our connections
            it->second.update();
            if (it->second.getStation() == disconnected) {
                // the connection timed out
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
                connection->second.receive(received->packet);
            } else {
                Logger::logInfo("creating a new connection");
                //create a new connection
                connections.insert(std::pair<std::string, Connection>(
                    addr, Connection(socket)));
                auto& newConnection = connections.find(addr)->second;
                newConnection.connect(received->from.ip, received->from.port);
                newConnection.receive(received->packet);
            }
        }

        //sendInput();
    }
}

void Server::sendMessage(const std::string& buffer){
    for(auto it = connections.begin(); it != connections.end(); it++){
        it->second.sendMessage(buffer);
    }
}

void Server::sendLatestMessage(const std::string& buffer){
    for(auto it = connections.begin(); it != connections.end(); it++){
        it->second.sendLatestMessage(buffer);
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

        // add a chance to drop packets for testing purposes
        /*std::random_device r;
        std::default_random_engine e1(r());
        std::uniform_real_distribution<double> distribution(0.);
        if (distribution(e1) < Globals::packetLoss) {
            // Logger::logInfo("Dropping packet: " + std::to_string(receivedPackets));
            return {};
        }*/

        return (MessageFrom){(ConnectID){from->ip, from->port}, packet};
    }

    return {};
}

void Server::sendInput() {
    /*bool hasInput = false;
#if PLATFORM == PLATFORM_WINDOWS
    // one issue with this windows implementation is that if the user starts
    // typing the server will stop sending messages until the user finishes
    // typing this will lead to a timeout if the user takes more than 10
    // seconds to type
    HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
    unsigned long numEvents, numRead;
    GetNumberOfConsoleInputEvents(stdIn, &numEvents);

    std::unique_ptr<INPUT_RECORD> records(new INPUT_RECORD[numEvents]);
    PeekConsoleInput(stdIn, records.get(), numEvents, &numRead);

    if (numRead != numEvents)
        std::cout << "What the actual fuck" << std::endl;

    for (int i = 0; i < numEvents; i++) {
        if (records.get()[i].EventType == KEY_EVENT) {
            hasInput = true;
            break;
        }
    }
#else
    fd_set fds = {};
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 0;
    int result = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if (result && result != -1 && FD_ISSET(STDIN_FILENO, &fds))
        hasInput = true;
#endif

    if (hasInput) {
        // there is something to read
        std::getline(std::cin, message);
        std::cin.clear();
        if (message.length() && connectionEstablished) {
            std::vector<Protocol::AppData> data;
            data.push_back(Protocol::AppData(currentMessage++, message));
            send(ip, clientPort, data);
            lastSentTime = std::chrono::high_resolution_clock::now();
        }

#if PLATFORM == PLATFORM_WINDOWS
        FlushConsoleInputBuffer(stdIn);
#endif
    }*/
}

void Server::manageMode() {
    /*auto now = std::chrono::high_resolution_clock::now();
    // time elapsed in mode
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - modeStart)
                       .count();

    // check rtt, and if it is bad, switch to a bad sendrate
    if (mode == Globals::ConnectionState::GOOD && rtt > Globals::badRtt) {
        // immediately drop to bad mode
        mode = Globals::ConnectionState::BAD;
        modeStart = std::chrono::high_resolution_clock::now();

        std::cout << "Entering bad mode!" << std::endl;

        if (elapsed < 10000) {
            returnToGood *= 2;
            if (returnToGood > 60000)
                returnToGood = 60000;
        }
    }

    if (mode == Globals::ConnectionState::BAD && elapsed > returnToGood) {

        std::cout << "Entering good mode!" << std::endl;

        // return to good after returnToGood time has passed
        mode = Globals::ConnectionState::GOOD;
        lastTrustTime = modeStart =
            std::chrono::high_resolution_clock::now();
    }

    auto timeSinceTrust =
        std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                              lastTrustTime)
            .count();
    if (mode == Globals::ConnectionState::GOOD && timeSinceTrust > 10000) {
        // trust the connection
        returnToGood /= 2;
        lastTrustTime = std::chrono::high_resolution_clock::now();
        if (returnToGood < 1000)
            returnToGood = 1000;
    }*/
}
