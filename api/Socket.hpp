#pragma once 

#include "PlatformDetection.hpp"

#if PLATFORM == PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <WS2tcpip.h>
#elif PLATFORM == PLATFORM_MAC || PLATFORM_UNIX
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

#include "Globals.hpp"

#include <string>
#include <optional>
#include <utility>

//this class assumes it is the only way sockets are created in an application
//might change this
class Socket {
public:
    Socket(int _domain, int _type);
    Socket(int sockfd);

    ~Socket();

    static bool initialize();
    static void shutdown();

    bool setNonBlocking();

    void bind(unsigned short port);
    bool listen();
    std::optional<Socket> accept();

    bool connect(std::string ip, int port);

    void send(std::string message);
    std::string receive();
    void receive(void* data, int length);

    bool sendTo(std::string ip, int port, std::string message);
    std::optional<std::pair<std::string, unsigned int>> receiveFrom(std::unique_ptr<char>* data);

    inline int getSocket(){
        return sockfd;
    }
private:
    int domain, type;

    int sockfd;
    struct sockaddr_in addr;
    socklen_t addrlen;

    static bool initialized;
    //count instances so we can automatically call clearnup
    static int instances;
};
