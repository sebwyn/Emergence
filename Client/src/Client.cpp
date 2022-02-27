#include "Client.hpp"
#include "PlatformDetection.hpp"

#include "Globals.hpp"
#include "Protocol.hpp"

#include <fcntl.h>
#include <iostream>

#if PLATFORM == PLATFORM_WINDOWS
#include <windows.h>

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

#define STDIN_FILENO _fileno(stdin)
#else
#include <sys/select.h>
#include <unistd.h>
#endif

Client::Client() : socket(AF_INET), connection(socket) {

    // open on an available port
    socket.bind(0);
    socket.setNonBlocking();
}

void Client::connect(const std::string &ip){
    std::string temp("");
    connection.connect(ip, Globals::port, temp);
}

void Client::connect(const std::string &ip, const std::string &message){
    connection.connect(ip, Globals::port, message);
}

void Client::update() {
    //if there isn't an active connection return
    if (connection.getStation() == disconnected) {
        return;
    }

    connection.update();

    // receive messages first (so we know what has been acknowledged)
    auto received = receive();
    if (received.has_value()) {
        if (received->from.ip == connection.getIp() &&
            received->from.port == connection.getPort()) {
            connection.receive(received->packet);
        } else {
            Logger::logError("Receiving messages not from server, very strange indeed!");
        }
    }

    //sendInput();
}

std::optional<MessageFrom> Client::receive() {
    std::string received;
    auto from = socket.receiveFrom(received);
    if (from.has_value()) {
        // the message actually has content
        Protocol::Packet packet(received);

        // ensure the message is using our protocol
        if (std::string(packet.header.protocol) != Globals::protocol())
            return {};

        // add a chance to drop packets for testing purposes
        return (MessageFrom){(ConnectID){from->ip, from->port}, packet};
    }

    return {};
}

void Client::sendInput() {
    bool hasInput = false;
#if PLATFORM == PLATFORM_WINDOWS
    // one issue with this windows implementation is that if the user starts
    // typing the server will stop sending messages until the user finishes
    // typing this will lead to a timeout if the user takes more than 10 seconds
    // to type
    HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
    unsigned long numEvents, numRead;
    GetNumberOfConsoleInputEvents(stdIn, &numEvents);

    std::unique_ptr<INPUT_RECORD> records(new INPUT_RECORD[numEvents]);
    PeekConsoleInput(stdIn, records.get(), numEvents, &numRead);

    if (numRead != numEvents)
        Logger::logError("What the actual fuck");

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
        if (message.length()) {
            connection.sendMessage(message);
        }

#if PLATFORM == PLATFORM_WINDOWS
        FlushConsoleInputBuffer(stdIn);
#endif
    }
}
