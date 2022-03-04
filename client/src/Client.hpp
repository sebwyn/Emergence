#pragma once

#include "Connection.hpp"
#include "Protocol.hpp"
#include "Socket.hpp"

// in essence a client adds almost nothing on top
// of a connection, but I still have it here, just for the sake of it
// the class is still kind of the couternpart of the server
// which is necessary because it maintains many connections
class Client {
  public:
    Client();

    void connect(const std::string &ip, const std::string &message = "") {
        if(connection.getRunning() == false){
            connection.connect(ConnectId(ip, Globals::port), message);
            //on connect start the server
            connection.start();
        }
    }

    void disconnect(){
        connection.disconnect();
    }

    void join(){
        connection.join();
    }

    void update();
    void sendMessage(const std::string &message){
        connection.sendMessage(message);
    }

    std::vector<Protocol::AppData> getMessages() {
        return connection.getMessages();
    }
    void flushMessages() { connection.flushMessages(); }

    Station getStation() { return connection.getStation(); }

  private:
    std::optional<MessageFrom> receive();

    UdpSocket socket;
    Connection connection;
};
