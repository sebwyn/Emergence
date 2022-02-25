#include "Server.hpp"

#include "Globals.hpp"

#include <iostream>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>

    void usleep(__int64 usec) { 
        HANDLE timer; 
        LARGE_INTEGER ft; 

        ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

        timer = CreateWaitableTimer(NULL, TRUE, NULL); 
        SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
        WaitForSingleObject(timer, INFINITE); 
        CloseHandle(timer); 
    }

    #define STDIN_FILENO _fileno(stdin)
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <unistd.h>
#endif
#include <random>

Server::Server() : socket(AF_INET) {
    socket.bind(Globals::port);
    socket.setNonBlocking();

    //flush the input on windows so it doesn't wait for the next input
    #ifdef PLATFORM_WINDOWS
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    #endif

    receivedPackets.set();
}

void Server::update(){

    //receive messages and ack
    lastSentTime = std::chrono::high_resolution_clock::now();
    while(message != "quit"){
        auto updateStart = std::chrono::high_resolution_clock::now();

        if(connectionEstablished){
            //timeout the connection after timeout length
            if(std::chrono::duration_cast<std::chrono::seconds>(updateStart - lastPacketTime).count() > Globals::timeout){
                std::cout << "The connection timed out!" << std::endl;
                break;
            }

            //send a message to the client every second that keeps the connection alive, and acks
            if(std::chrono::duration_cast<std::chrono::milliseconds>(updateStart - lastSentTime).count() > (1000 / Globals::goodSendRate)){
                sendKeepAlive(ip, clientPort);
                lastSentTime = updateStart;
            }

            manageMode();
        }

        //receive a valid packet per our protocol, and establish a connection if 
        //one is not already established
        auto received = receive();
        if(received.has_value()){
            lastPacketTime = std::chrono::high_resolution_clock::now();

            //not a keep alive
            if(received->messages.size()){
                for(Globals::AppData message : received->messages){
                    std::cout << "C:" << message.toString() << std::endl;
                }
            }
        }

        //connection established check is contained within,
        //so input continues to get read even if there is no connection
        //however data is only sent if there is a connection
        sendInput();

        //all this update time stuff may be a little unnecessary because it takes less than a ms
        auto updateFinish = std::chrono::high_resolution_clock::now();
        auto updateTime = std::chrono::duration_cast<std::chrono::milliseconds>(updateFinish - updateStart).count();
        //std::cout << "updating took: " << updateTime << std::endl;
        //sleep for fps - updateTime so that we consistently update to maintain fps
        usleep( 1000 / Globals::fps - updateTime);
    }
}

void Server::sendKeepAlive(std::string ip, ushort port){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header);

    sentPackets.insert(std::pair<uint, Globals::PacketHandled>(header.seq, Globals::PacketHandled(packet, [](Globals::PacketHandled ph){})));
    socket.sendTo(ip, port, packet.toBuffer());
}

void Server::send(std::string ip, int port, std::vector<Globals::AppData>& messages, std::function<void(Globals::PacketHandled)> onResend){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header, messages);

    sentPackets.insert(std::pair<uint, Globals::PacketHandled>(header.seq, Globals::PacketHandled(packet, onResend)));
    socket.sendTo(ip, port, packet.toBuffer());
}

std::optional<Globals::Packet> Server::receive(){
    std::unique_ptr<char> received;
    auto from = socket.receiveFrom(&received);
    if(from.has_value()){
        //the message actually has content
        Globals::Packet packet(received);

        //ensure the message is using our protocol
        if(std::string(packet.header.protocol) != Globals::protocol())
            return {};

        if(!connectionEstablished){
            connectionEstablished = true;
            ip = from->first;
            clientPort = from->second;
            lastSentTime = std::chrono::high_resolution_clock::now();
        } else if(from->first != ip){
            //drop the packet if it isn't coming from the one client
            //TODO: make it so another connection is created
            return {};
        }

        //add a chance to drop packets for testing purposes
        std::random_device r;
        std::default_random_engine e1(r());
        std::uniform_real_distribution<double> distribution(0.);
        if(distribution(e1) < Globals::packetLoss){
            //std::cout << "Dropping packet: " << receivedPackets << std::endl;
            return {};
        }

        handlePacket(packet);

        return packet;
    }
    return {};
}

bool Server::acked(Globals::Header header, uint seq){
    if(header.ack == seq){
        return true;
    } else {
        if(seq < header.ack && header.ack - seq <= 32){
            return receivedPackets.test(header.ack - seq - 1);
        }
    }
    return false;
}

//the only issue i see is this relies on us receiving packets in order 
//to resend lost packets
void Server::handlePacket(Globals::Packet received){
    //rotate our received bitset by the difference between remoteSeqNum and receivedSeqNum
    //then set the bitset so the previous remoteSeqNum is acked

    //keep in mind packets may arrive out of order so sequence number may be less than our remote
    //sequence number
    if(received.header.seq > remoteSeqNum){
        receivedPackets = receivedPackets << received.header.seq - remoteSeqNum;
        //ack the previous remote seq num
        receivedPackets.set(received.header.seq - remoteSeqNum - 1);
        remoteSeqNum = received.header.seq;
    } else if(received.header.seq < remoteSeqNum && (remoteSeqNum - received.header.seq) < 32) {
        receivedPackets.set(remoteSeqNum - received.header.seq - 1);
    }

    //iterate over 
    for(auto it = sentPackets.cbegin(); it != sentPackets.cend(); ){
        Globals::PacketHandled packetHandled = it->second;
        //regardless of whether or not its been acked calculate time since sending message
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - packetHandled.sent).count();
        if(acked(received.header, it->first)){
            if(!rttDefined){
                rtt = elapsed;
                rttDefined = true;
            } else {
                rtt += Globals::rttSmooth * elapsed;
            }
            //if its been acked remove from list
            sentPackets.erase(it++);
            continue;
        } else {
            //if its been over resend time, resend the packet
            if(elapsed > Globals::packetLostTime){
                //The packet was lost
                packetHandled.onResend(packetHandled);

                //remove from list
                sentPackets.erase(it++);
                continue;
            }

        }
        ++it;
    }
}

void Server::sendInput(){
    bool hasInput = false;
    #ifdef PLATFORM_WINDOWS
        //one issue with this windows implementation is that if the user starts typing 
        //the server will stop sending messages until the user finishes typing
        //this will lead to a timeout if the user takes more than 10 seconds to type
        HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
        unsigned long numEvents, numRead;
        GetNumberOfConsoleInputEvents(stdIn, &numEvents);

        std::unique_ptr<INPUT_RECORD> records(new INPUT_RECORD[numEvents]);
        PeekConsoleInput(stdIn, records.get(), numEvents, &numRead);

        if(numRead != numEvents)
            std::cout << "What the actual fuck" << std::endl;


        for(int i = 0; i < numEvents; i++){
            if(records.get()[i].EventType == KEY_EVENT){
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
        int result = select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
        if(result && result != -1 && FD_ISSET(STDIN_FILENO, &fds)) hasInput = true;
    #endif

    if(hasInput){
        std::cout << "yup has input" << std::endl;
        //there is something to read
        std::getline(std::cin, message);
        std::cin.clear();
        if(message.length() && connectionEstablished){
            std::vector<Globals::AppData> data;
            data.push_back(Globals::AppData(currentMessage++, message));
            send(ip, clientPort, data);
            lastSentTime = std::chrono::high_resolution_clock::now();
        }

        #ifdef PLATFORM_WINDOWS
            FlushConsoleInputBuffer(stdIn);
        #endif
    }
}

void Server::manageMode(){
    auto now = std::chrono::high_resolution_clock::now();
    //time elapsed in mode
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - modeStart).count();

    //check rtt, and if it is bad, switch to a bad sendrate
    if(mode == Globals::ConnectionState::GOOD && rtt > Globals::badRtt){
        //immediately drop to bad mode
        mode = Globals::ConnectionState::BAD;
        modeStart = std::chrono::high_resolution_clock::now();

        std::cout << "Entering bad mode!" << std::endl;

        if(elapsed < 10000){
            returnToGood *= 2;
            if(returnToGood > 60000) returnToGood = 60000;
        }
    }

    if(mode == Globals::ConnectionState::BAD && elapsed > returnToGood){

        std::cout << "Entering good mode!" << std::endl;

        //return to good after returnToGood time has passed
        mode = Globals::ConnectionState::GOOD;
        lastTrustTime = modeStart = std::chrono::high_resolution_clock::now();
    }

    auto timeSinceTrust = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTrustTime).count();
    if(mode == Globals::ConnectionState::GOOD && timeSinceTrust > 10000){
        //trust the connection
        returnToGood /= 2;
        lastTrustTime = std::chrono::high_resolution_clock::now();
        if(returnToGood < 1000) returnToGood = 1000;
    }
}