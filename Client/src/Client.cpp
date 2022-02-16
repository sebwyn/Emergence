#include "Client.hpp"

#include "Globals.hpp"

#include <iostream>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>

Client::Client() : socket(AF_INET, SOCK_DGRAM) {
    socket.bind(Globals::port+1);
    socket.setNonBlocking();

    std::cout << "What ip would you like to connect to: ";
    std::cin >> ip;

    //set receivedPackets to high
    receivedPackets.set();
}

void Client::update(){
    std::string message;
    while(message != "quit"){
        //check to see if the connection has timed out
        auto updateStart = std::chrono::high_resolution_clock::now();
        
        if(connectionEstablished){
            if(std::chrono::duration_cast<std::chrono::seconds>(updateStart - lastPacketTime).count() > 10){
                std::cout << "The connection timed out!" << std::endl;
                break;
            }

            if(std::chrono::duration_cast<std::chrono::seconds>(updateStart - lastSentTime).count() > 1){
                //send a keep alive
                sendKeepAlive();
                lastSentTime = std::chrono::high_resolution_clock::now();
            }

            resendPackets();
        }

        //receive messages first (so we know what has been acknowledged)
        auto received = receive();
        if(received.has_value()){
            if(!connectionEstablished)
                connectionEstablished = true;
        
            lastPacketTime = std::chrono::high_resolution_clock::now();
            
            updateSentPackets(*received);
            
            //not a keep alive
            if(received->messageCount){
                for(Globals::AppData message : received->messages){
                    std::cout << "H:" << message.toString() << std::endl;
                }
            }
        }

        sendInput();

        //all this update time stuff may be a little unnecessary because it takes less than a ms
        auto updateFinish = std::chrono::high_resolution_clock::now();
        auto updateTime = std::chrono::duration_cast<std::chrono::milliseconds>(updateFinish - updateStart).count();
        //std::cout << "updating took: " << updateTime << std::endl;
        //sleep for fps - updateTime so that we consistently update to maintain fps
        usleep( 1000 / Globals::fps - updateTime);

    }
}

void Client::sendKeepAlive(){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header);

    socket.sendTo(ip, Globals::port, packet.toBuffer());
}

void Client::send(std::vector<Globals::AppData>& messages){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header, messages);

    socket.sendTo(ip, Globals::port, packet.toBuffer());
}

std::optional<Globals::Packet> Client::receive(){
    std::unique_ptr<char> received;
    auto from = socket.receiveFrom(&received);
    if(from.has_value() && from->first == ip && from->second == Globals::port){
        Globals::Packet packet(received);

        //ensure protocol
        if(std::string(packet.header.protocol) != Globals::protocol())
            return {};

        return packet;
    }
    return {};
}

void Client::resendPackets(){
    //iterate over all the sent packets, checking if any have been lost and resending
    auto i = sentAppData.begin();
    for(Globals::AppDataHandled packet : sentAppData){
        //if the packet is still in this list after a second, resend it  
        auto now = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - packet.sent).count() > Globals::packetLostTime){
            std::cout << "Resending packet: " << packet.data.toString() << " after: " << std::chrono::duration_cast<std::chrono::milliseconds>(now - packet.sent).count() << std::endl;
            packet.onResend(packet.data);
            packet.resent = true;
            //remove from the list
            if(std::next(i) == sentAppData.cend()){
                sentAppData.erase(i);
                break;
            }
            sentAppData.erase(i);
        }  
        ++i; 
    }
}

void Client::updateSentPackets(Globals::Packet received){
    //rotate our received bitset by the difference between remoteSeqNum and receivedSeqNum
    //then set the bitset so the previous remoteSeqNum is acked

    //keep in mind packets may arrive out of order so sequence number may be less than our remote
    //sequence number
    if(received.header.seq > remoteSeqNum){
        receivedPackets << received.header.seq - remoteSeqNum;
        //ack the previous remote seq num
        receivedPackets.set(received.header.seq - remoteSeqNum - 1);
        remoteSeqNum = received.header.seq;
    } else if(received.header.seq < remoteSeqNum && (remoteSeqNum - received.header.seq) < 32) {
        receivedPackets.set(remoteSeqNum - received.header.seq - 1);
    }

    //update sentPackets, acking anything that has been acked
    auto i = sentAppData.cbegin();
    while(i != sentAppData.cend()){
        if(received.header.ack == i->seq){
            //remove from the list, problematic if the last element
            if(std::next(i) == sentAppData.cend()){
                sentAppData.erase(i);
                break;
            }
            sentAppData.erase(i);
        }

        if(i->seq < received.header.ack){
            if(i->seq - received.header.ack <= 32){
                if(received.header.acks.test(i->seq - received.header.ack - 1)){
                    if(std::next(i) == sentAppData.cend()){
                        sentAppData.erase(i);
                        break;
                    }
                    sentAppData.erase(i);
                } else {
                    std::cout << "Damn that wasn't received!" << std::endl;
                }
            } else {
                if(!i->resent){
                    auto now = std::chrono::high_resolution_clock::now();
                    std::cout << "Seriously not acked after 32 packets? and " << std::chrono::duration_cast<std::chrono::milliseconds>(now - i->sent).count() << " ms and " << i->seq << " " << remoteSeqNum << std::endl;
                    //drop the packet out of the queue
                    if(std::next(i) == sentAppData.cend()){
                        sentAppData.erase(i);
                        break;
                    }
                    sentAppData.erase(i);
                } else {
                    std::cout << "Something is whacky, not resent and over 32 acks" << std::endl;
                }
            }
        }
        ++i;
    }
}

void Client::sendInput(){
    std::string message;

    //then send messages from queue
    fd_set fds;
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 0;
    int result = select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
    if(result && result != -1 && FD_ISSET(STDIN_FILENO, &fds)){
        //there is something to read
        std::getline(std::cin, message);
        std::cin.clear();
        if(message.length()){
            std::vector<Globals::AppData> data;
            Globals::AppData m = Globals::AppData(currentMessage++, message);
            data.push_back(m);

            if(connectionEstablished){
                sentAppData.push_back(Globals::AppDataHandled(localSeqNum, m, 
                    [](Globals::AppData data){ 
                        //std::cout << "dropped message" << std::endl;
                    }
                ));
            }

            send(data);
            lastSentTime = std::chrono::high_resolution_clock::now();
        }
    }
}