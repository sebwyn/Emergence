#pragma once

#include <bitset>
#include <sstream>
#include <vector>
#include <iostream>

class Globals {
public:
    static const int maxMsgLength = 1024;

    static const int port = 1337;
    //time before packet resend is called when packet hasn't been acked in ms
    static const int packetLostTime = 1000;
    //float percentage of packet being lost
    static constexpr double packetLoss = .02;
    static std::string protocol(){
        return std::string("MERG");
    }
    static constexpr int fps = 60;

    //time before connection times out in seconds
    static constexpr int timeout = 10;
    
    //for congestion avoidance
    static constexpr float rttSmooth = 0.1f;
    static constexpr int goodSendRate = 30;
    static constexpr int badSendRate = 10;
    static constexpr int badRtt = 250;

    enum ConnectionState {
        GOOD,
        BAD,
    };

    static void writeUintToBuf(std::string& buffer, uint n){
        uint offset = buffer.length();
        buffer += std::string(sizeof(uint), ' ');
        memcpy(buffer.data() + offset, &n, sizeof(uint));
    }

    static uint readUintFromBuf(char* address){
        uint out;
        memcpy(&out, address, sizeof(uint));
        return out;
    }

    struct AppData {
        uint id;
        std::string message;

        AppData() : id(0), message(std::string("")) {}
        AppData(uint id, std::string message) : id(id), message(message) {}
        AppData(char* address){
            uint offset = 0;

            uint messageSize = readUintFromBuf(address);
            offset += sizeof(uint);
            id = readUintFromBuf(address+offset);
            offset += sizeof(uint);
            message = std::string(address+offset, messageSize);
        }

        std::string toString(){
            return "(Data: " + std::to_string(id) + ", " + message + ")";
        }
        std::string toBuffer(){
            std::string buffer = std::string(sizeof(uint), ' ');
            memcpy(buffer.data(), &id, sizeof(uint));
            buffer += message;
            return buffer;
        }
    };

    struct Header {
        char protocol[5] = {'M', 'E', 'R', 'G', '\0'};
        uint seq;
        uint ack;
        std::bitset<32> acks;

        Header() : seq(0), ack(0) {}
        Header(uint seq, uint ack, std::bitset<32> acks) : seq(seq), ack(ack), acks(acks) {
        }
        Header(char* address){
            memcpy(this, address, sizeof(Header));
        }

        std::string toString(){
            return "(Header: " + std::to_string(seq) + ", " + std::to_string(ack) + ", " + acks.to_string() + ")";
        }

        std::string toBuffer(){
            std::string buffer = std::string(sizeof(Header), ' ');
            memcpy(buffer.data(), this, sizeof(Header));
            return buffer;
        }

    };

    struct Packet { 
        Header header;
        uint messageCount;
        //messages are interleaved with uint size
        std::vector<AppData> messages;

        Packet(Header header) : header(header), messageCount(0) {}
        Packet(Header header, std::vector<AppData>& messages) : header(header), messages(messages), messageCount(messages.size()) {}
        Packet(std::unique_ptr<char>& buffer){
            
            uint offset = sizeof(header);

            header = Header(buffer.get());
            messageCount = readUintFromBuf(buffer.get()+offset);
            offset += sizeof(uint);

            for(int i = 0; i < messageCount; i++){
                AppData data = AppData(buffer.get()+offset);
                messages.push_back(data);
                offset += 2*sizeof(uint) + data.message.length();
            }
        }

        std::string toString(){
            std::string out;
            out = "(Packet: " + header.toString();
            for(AppData message : messages){
                out += ", " + message.toString();
            }
            out += ")";
            return out;
        }

        std::string toBuffer(){
            std::string buffer = header.toBuffer();
            writeUintToBuf(buffer, messageCount);
            for(AppData message : messages){
                //copy the messageLength and the message into the buffer for every message
                std::string m = message.toBuffer();
                uint messageLength = m.length();
                writeUintToBuf(buffer, messageLength);
                buffer += m;
            }
            //std::cout << "Produced buffer: " << buffer << std::endl;
            return buffer;
        }
    };

    struct PacketHandled {
        Packet packet;
        std::chrono::high_resolution_clock::time_point sent;
        std::function<void(PacketHandled)> onResend;

        PacketHandled(Packet packet, std::function<void(PacketHandled)> onResend) : packet(packet), onResend(onResend) {
            sent = std::chrono::high_resolution_clock::now();
        }
    };
};