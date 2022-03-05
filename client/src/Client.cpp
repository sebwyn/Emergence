#include "Client.hpp"
#include "PlatformDetection.hpp"

#include "Globals.hpp"
#include "Protocol.hpp"

#include <fcntl.h>

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

void Client::update() {
    //if there isn't an active connection return
    if(connection.getRunning() == false){
        connection.tryJoin();
        return;
    }

    // receive messages first (so we know what has been acknowledged)
    auto received = receive();
    while(received.has_value()) {
        if(received->from == connection.getOther()){
            //Logger::logInfo(received->packet.toBuffer());
            connection.pushToReceive(received->packet);
        } else {
            Logger::logError("Receiving messages not from server, very strange indeed!");
        }

        received = receive();
    }

    //sendInput();
}

//this is repeated code, think about how to clean this up
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
        return (MessageFrom){(ConnectId){from->ip, from->port}, packet};
    }

    return {};
}
