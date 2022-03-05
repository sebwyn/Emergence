#include "Socket.hpp"

#include "PlatformDetection.hpp"

// include socket lib on windows
#if PLATFORM == PLATFORM_WINDOWS
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

#if PLATFORM == PLATFORM_WINDOWS

#include <WS2tcpip.h>
#include <winsock2.h>

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

bool Socket::initialized = false;
int Socket::instances = 0;

bool Socket::initialize() {
#if PLATFORM == PLATFORM_WINDOWS
    WSADATA WsaData;
    return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
#else
    return true;
#endif
}

void Socket::shutdown() {
#if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
#endif
}

Socket::Socket(int _domain, int _type) : domain(_domain), type(_type) {
    if (!initialized) {
        if (!initialize()) {
            Logger::logError("Failed to initialize sockets!");
        }
        initialized = true;
    }

    if ((sockfd = socket(domain, type, 0)) == 0) {
        Logger::logError("Failed to create socket!");
    }
    instances++;
}

Socket::Socket(int sockfd) : sockfd(sockfd) { instances++; }

Socket::~Socket() {
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    close(sockfd);
#elif PLATFORM == PLATFORM_WINDOWS
    closesocket(sockfd);
#endif

    instances--;
    if (instances == 0) {
        shutdown();
    }
}

bool Socket::setNonBlocking() {
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

    int nonBlocking = 1;
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
        printf("failed to set non-blocking\n");
        return false;
    }

#elif PLATFORM == PLATFORM_WINDOWS

    DWORD nonBlocking = 1;
    if (ioctlsocket(sockfd, FIONBIO, &nonBlocking) != 0) {
        printf("failed to set non-blocking\n");
        return false;
    }

#endif

    return true;
}

void Socket::bind(unsigned short port) {
    int opt = 1;

    addr.sin_family = domain;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        Logger::logError("Binding socket failed!");
    }
}

bool TcpServer::listen() {
    if (::listen(sockfd, 3) < 0) {
        Logger::logError("Listen failed: " + std::to_string(errno));
        return false;
    }
    return true;
}

std::optional<TcpServer> TcpServer::accept() {
    int comsock;
    if ((comsock = ::accept(sockfd, (struct sockaddr *)&addr, &addrlen)) < 0) {
        Logger::logError("Could not accept connection: " +
                         std::to_string(errno));
        return {};
    }
    return TcpServer(comsock);
}

bool TcpClient::connect(const std::string ip, int port) {
    addr.sin_family = domain;
    addr.sin_port = htons(port);

    if (inet_pton(domain, ip.c_str(), &addr.sin_addr) <= 0) {
    Logger::logError("Invalid ip: " + std::to_string(errno));
        return false;
    }

    if (::connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    Logger::logError("Connection Failed: " + std::to_string(errno));
        return false;
    }
    return true;
}

void TcpConnection::send(std::string message) {
    ::send(sockfd, message.c_str(), message.length(), 0);
}

std::string TcpConnection::receive() {
    char buffer[Globals::maxMsgLength] = {0};
    recv(sockfd, buffer, Globals::maxMsgLength, 0);
    return std::string(buffer);
}

std::optional<Socket::ReceivedInfo>
UdpSocket::receiveFrom(std::string &message) {

    sockaddr_in from;
    socklen_t fromLen = sizeof(from);

    std::unique_ptr<char> data = std::unique_ptr<char>(new char[Globals::maxMsgLength]);
    memset(data.get(), 0, Globals::maxMsgLength);
    int bytes = recvfrom(sockfd, data.get(), Globals::maxMsgLength, 0,
                         (sockaddr *)&from, &fromLen);

    if (bytes <= 0) {
        return {};
    }

    message = std::string(data.get(), bytes); 

    unsigned int from_address = from.sin_addr.s_addr;
    unsigned short from_port = ntohs(from.sin_port);

    char ipBuffer[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(domain, &from_address, ipBuffer, INET_ADDRSTRLEN) ==
        nullptr) {
        Logger::logError("inet_ntop failed: " + std::to_string(errno));
        return {};
    }

    return (ReceivedInfo){std::string(ipBuffer), from_port, bytes};
}

bool UdpSocket::sendTo(std::string ip, int port, std::string message) {

    sockaddr_in to;

    to.sin_family = domain;
    to.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &(to.sin_addr)) <= 0) {
        Logger::logError("inet_pton failed: " + std::to_string(errno));
        return false;
    }

    int bytesSent = sendto(sockfd, message.c_str(), message.length(), 0,
                           (const sockaddr *)&to, sizeof(to));
    if (bytesSent != message.length()) {
        Logger::logError("Couldn't send the whole message");
    }

    return true;
}
