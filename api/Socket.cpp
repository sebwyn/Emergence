#include "Socket.hpp"

#include "PlatformDetection.hpp"

//include socket lib on windows
#if PLATFORM == PLATFORM_WINDOWS
    #pragma comment( lib, "wsock32.lib" )
#endif

#if PLATFORM == PLATFORM_WINDOWS

    #include <winsock2.h>

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>

#endif

#include <iostream>
#include <cerrno>
#include <vector>

bool Socket::initialized = false;
int Socket::instances = 0;

bool Socket::initialize(){
    #if PLATFORM == PLATFORM_WINDOWS
        WSADATA WsaData;
        return WSAStartup( MAKEWORD(2,2), 
                           &WsaData ) 
            == NO_ERROR;
    #else
        return true;
    #endif
}

void Socket::shutdown(){
    #if PLATFORM == PLATFORM_WINDOWS
        WSACleanup();
    #endif
}

Socket::Socket(int _domain, int _type) : domain(_domain), type(_type) {
    if(!initialized){
        if(!initialize()){
            std::cout << "Failed to initialize sockets!" << std::endl;
        }
        initialized = true;
    }
    
    if((sockfd = socket(domain, type, 0)) == 0){
        std::cout << "Failed to create socket!" << std::endl;
    }
    instances++;
}

Socket::Socket(int sockfd) : sockfd(sockfd) {
    instances++;
}

Socket::~Socket(){
    #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
        close(sockfd);
    #elif PLATFORM == PLATFORM_WINDOWS
        closesocket(sockfd);
    #endif

    instances--;
    if(instances == 0){
        shutdown();
    }
}

bool Socket::setNonBlocking(){
    #if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

        int nonBlocking = 1;
        if ( fcntl( sockfd, 
                    F_SETFL, 
                    O_NONBLOCK, 
                    nonBlocking ) == -1 )
        {
            printf( "failed to set non-blocking\n" );
            return false;
        }

    #elif PLATFORM == PLATFORM_WINDOWS

        DWORD nonBlocking = 1;
        if ( ioctlsocket( handle, 
                          FIONBIO, 
                          &nonBlocking ) != 0 )
        {
            printf( "failed to set non-blocking\n" );
            return false;
        }

    #endif

    return true;
}

void Socket::bind(int port){
    int opt = 1;

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        std::cout << "Failed to set socket options!" << std::endl;
    }

    addr.sin_family = domain;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        std::cout << "Binding socket failed!" << std::endl;
    }
}

bool Socket::listen(){
    if(::listen(sockfd, 3) < 0){ 
        std::cout << "Listen failed: " << errno << std::endl;
        return false;
    }
    return true;
}

std::optional<Socket> Socket::accept(){
    int comsock;
    if((comsock = ::accept(sockfd, (struct sockaddr*)&addr, &addrlen)) < 0){
        std::cout << "Could not accept connection: " << errno << std::endl;
        return {};
    }
    return Socket(comsock);
}

bool Socket::connect(const std::string ip, int port){
    addr.sin_family = domain;
    addr.sin_port = htons(port);

    if(inet_pton(domain, ip.c_str(), &addr.sin_addr) <= 0){
        std::cout << "Invalid ip: " << errno << std::endl;
        return false;
    }

    if(::connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        std::cout << "Connection Failed: " << errno << std::endl;
        return false;
    }
    return true;
}

void Socket::send(std::string message){
    ::send(sockfd, message.c_str(), message.length(), 0);
}

std::string Socket::receive(){
    char buffer[MAX_MESG_LENGTH] = {0};
    recv(sockfd, buffer, MAX_MESG_LENGTH, 0);
    return std::string(buffer);
}

std::optional<std::pair<std::string, unsigned int>> Socket::receiveFrom(char** data){

    sockaddr_in from;
    socklen_t fromLen = sizeof(from);

    int bytes = recvfrom(sockfd, data, MAX_MESG_LENGTH, 0, (sockaddr*)&from, &fromLen);

    if(bytes <= 0){
        return {};
    }

    unsigned int from_address = ntohl(from.sin_addr.s_addr);
    unsigned int from_port = ntohs(from.sin_port);

    char ipBuffer[INET_ADDRSTRLEN] = {0};
    if(inet_ntop(domain, &from_address, ipBuffer, INET_ADDRSTRLEN) == nullptr){
        std::cout << "inet_pton failed: " << errno << std::endl;
        return {};
    }

    return std::pair<std::string, unsigned int>(std::string(ipBuffer), from_port);
}

bool Socket::sendTo(std::string ip, int port, std::string message){
    
    sockaddr_in to;

    to.sin_family = domain;
    to.sin_port = htons(port); 

    if(inet_pton(AF_INET, ip.c_str(), &(to.sin_addr)) <= 0){
        std::cout << "inet_pton failed: " << errno << std::endl;
        return false;
    }
    
    int bytesSent = sendto(sockfd, message.c_str(), message.length(), 0, (const sockaddr*)&to, sizeof(to));
    //std::cout << "sent " << bytesSent << " bytes" << std::endl;

    return true;
}
