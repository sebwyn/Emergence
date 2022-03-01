#pragma once

#include "Connection.hpp"
#include "Protocol.hpp"
#include "Socket.hpp"

#include <chrono>
#include <deque>
#include <map>
#include <vector>

// in essence a client adds almost nothing on top
// of a connection, but I still have it here, just for the sake of it
// the class is still kind of the couternpart of the server
// which is necessary because it maintains many connections
class Client {
  public:
    Client();

    void connect(const std::string &ip);
    void connect(const std::string &ip, const std::string &message);

    void update();
    std::optional<MessageFrom> receive();

    void sendLatestMessage(const std::string &message) {
        connection.sendLatestMessage(message);
    }

    std::vector<Protocol::AppData> getMessages(){
        return connection.getMessages(); 
    }

    void flushMessages(){
        connection.flushMessages();
    }

    Station getStation() { return connection.getStation(); }

  private:
    // functions called once; defined to cleanup update loop
    void sendInput();
    std::string message = "";



    UdpSocket socket;
    Connection connection;
};
