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
#include "Logger.hpp"

#include <string>
#include <optional>
#include <utility>

//used so we can have multiple overloads
//kinda jank
struct socketfd {
    int sockfd;
};

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

    inline int getSocket(){
        return sockfd;
    }
protected:

    struct ReceivedInfo {
        std::string ip;
        ushort port;
        int length;
    };

    int domain, type;

    int sockfd;
    struct sockaddr_in addr;
    socklen_t addrlen;

    static bool initialized;
    //count instances so we can automatically call clearnup
    static int instances;
};

class TcpConnection : public Socket {
public:
    TcpConnection(int family) : Socket(family, SOCK_STREAM) {}
    TcpConnection(socketfd fd) : Socket(fd.sockfd) {}

    void send(std::string message);
    std::string receive();
    void receive(void* data, int length);
};

class TcpServer : public TcpConnection {
public:
    TcpServer(int family, ushort port) : TcpConnection(family) {
        bind(port);
    }
    TcpServer(int sockfd) : TcpConnection({sockfd}) {}

    bool listen();
    std::optional<TcpServer> accept();
};

class TcpClient : public TcpConnection {
public:
    TcpClient(int family);

    bool connect(std::string ip, int port);
};

class UdpSocket : public Socket {
public:
    UdpSocket(int family) : Socket(family, SOCK_DGRAM) {}

    bool sendTo(std::string ip, int port, std::string message);
    std::optional<ReceivedInfo> receiveFrom(std::string &message);
};
