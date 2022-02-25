#include "Client.hpp"

#include "Globals.hpp"

#include <iostream>
#include <fcntl.h>

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
    #include <sys/select.h>
    #include <unistd.h>
#endif

Client::Client() : socket(AF_INET, SOCK_DGRAM) {
    socket.bind(Globals::port+1);
    socket.setNonBlocking();

    std::cout << "What ip would you like to connect to: ";
    std::cin >> ip;

    //flush the input on windows so it doesn't wait for the next input
    #ifdef PLATFORM_WINDOWS
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    #endif

    //set receivedPackets to high
    receivedPackets.set();
}

void Client::update(){
    std::string message;
    lastTrustTime = modeStart = std::chrono::high_resolution_clock::now();
    while(message != "quit"){
        //check to see if the connection has timed out
        auto updateStart = std::chrono::high_resolution_clock::now();
        
        if(connectionEstablished){
            if(std::chrono::duration_cast<std::chrono::seconds>(updateStart - lastPacketTime).count() > Globals::timeout){
                std::cout << "The connection timed out!" << std::endl;
                break;
            }

            if(std::chrono::duration_cast<std::chrono::milliseconds>(updateStart - lastSentTime).count() > (1000 / Globals::goodSendRate)){
                //send a keep alive
                sendKeepAlive();
                lastSentTime = std::chrono::high_resolution_clock::now();
            }

            manageMode();
        }

        //receive messages first (so we know what has been acknowledged)
        auto received = receive();
        if(received.has_value()){
            if(!connectionEstablished) connectionEstablished = true;
            lastPacketTime = std::chrono::high_resolution_clock::now();
            
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

    sentPackets.insert(std::pair<uint, Globals::PacketHandled>(header.seq, Globals::PacketHandled(packet, [](Globals::PacketHandled ph){})));
    socket.sendTo(ip, Globals::port, packet.toBuffer());
}

void Client::send(std::vector<Globals::AppData>& messages, std::function<void(Globals::PacketHandled)> onResend){
    Globals::Header header(localSeqNum++, remoteSeqNum, receivedPackets);
    Globals::Packet packet(header, messages);

    sentPackets.insert(std::pair<uint, Globals::PacketHandled>(header.seq, Globals::PacketHandled(packet, onResend)));
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


        handlePacket(packet);

        return packet;
    }
    return {};
}

bool Client::acked(Globals::Header header, uint seq){
    if(header.ack == seq){
        return true;
    } else {
        if(seq < header.ack && header.ack - seq <= 32){
            return header.acks.test(header.ack - seq - 1);
        }
    }
    return false;
}

void Client::handlePacket(Globals::Packet received){
    //rotate our received bitset by the difference between remoteSeqNum and receivedSeqNum
    //then set the bitset so the previous remoteSeqNum is acked

    //keep in mind packets may arrive out of order so sequence number may be less than our remote
    //sequence number
    if(received.header.seq > remoteSeqNum){
        receivedPackets = receivedPackets << received.header.seq - remoteSeqNum;
        //ack the previous remote seq num
        receivedPackets.set(received.header.seq - remoteSeqNum - 1);
        remoteSeqNum = received.header.seq;
    } else if(received.header.seq < remoteSeqNum && (remoteSeqNum - received.header.seq) <= 32) {
        receivedPackets.set(remoteSeqNum - received.header.seq - 1);
    }

    auto now = std::chrono::high_resolution_clock::now();
    //iterate over 
    for(auto it = sentPackets.cbegin(); it != sentPackets.cend(); ){
        Globals::PacketHandled packetHandled = it->second;
        //regardless of whether or not its been acked calculate time since sending message
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - packetHandled.sent).count();
        if(acked(received.header, it->first)){
            if(!rttDefined){
                rtt = elapsed;
                rttDefined = true;
            } else {
                rtt += Globals::rttSmooth * (elapsed - rtt);
            }
            //if its been acked remove from list
            sentPackets.erase(it++);
            continue;
        } else {
            //if its been over resend time, resend the packet
            if(elapsed > Globals::packetLostTime){
                //The packet was lost
                if(packetHandled.packet.messageCount > 0){
                    std::cout << "Resending message: " << packetHandled.packet.messages[0].toString() << std::endl;
                }
                packetHandled.onResend(packetHandled);

                //remove from list
                sentPackets.erase(it++);
                continue;
            }

        }
        ++it;
    }
}

void Client::sendInput(){
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
        //there is something to read
        std::getline(std::cin, message);
        std::cin.clear();
        if(message.length()){
            std::vector<Globals::AppData> data;
            Globals::AppData m = Globals::AppData(currentMessage++, message);
            data.push_back(m);
            send(data);
            lastSentTime = std::chrono::high_resolution_clock::now();
        }

        #ifdef PLATFORM_WINDOWS
            FlushConsoleInputBuffer(stdIn);
        #endif
    }
}

void Client::manageMode(){
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