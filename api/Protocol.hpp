#pragma once

#include <bitset>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Protocol {
  public:
    struct AppData {
        uint id;
        // can contain null bytes, so is usable
        std::string message;

        AppData() : id(0), message(nullptr) {}
        AppData(uint id, std::string message) : id(id), message(message) {}
        AppData(const std::string &buffer, uint &offset) {
            id = readUintFromBuf(buffer, offset);
            uint length = readUintFromBuf(buffer, offset);
            message = std::string(buffer.data() + offset, length);
            offset += message.length();
        }

        std::string toString() {
            return "(Data: " + std::to_string(id) + ", " + message + ")";
        }
        std::string toBuffer() {
            std::string buffer = std::string(sizeof(uint), ' ');
            std::memcpy(buffer.data(), &id, sizeof(uint));
            buffer += std::string(sizeof(uint), ' ');
            uint length = message.length();
            std::memcpy(buffer.data() + sizeof(uint), &length, sizeof(uint));
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
        Header(uint seq, uint ack, std::bitset<32> acks)
            : seq(seq), ack(ack), acks(acks) {}
        Header(const std::string &buffer, uint &offset) {
            std::memcpy(this, buffer.data(), sizeof(Header));
            offset += sizeof(Header);
        }

        std::string toString() {
            return "(Header: " + std::to_string(seq) + ", " +
                   std::to_string(ack) + ", " + acks.to_string() + ")";
        }

        std::string toBuffer() {
            std::string buffer = std::string(sizeof(Header), ' ');
            std::memcpy(buffer.data(), this, sizeof(Header));
            return buffer;
        }
    };

    struct Packet {
        Header header;
        uint messageCount;
        // messages are interleaved with uint size
        std::vector<AppData> messages;

        Packet(Header header) : header(header), messageCount(0) {}
        Packet(Header header, std::vector<AppData> &messages)
            : header(header), messages(messages),
              messageCount(messages.size()) {}
        Packet(const std::string &buffer) {
            uint offset = 0;

            header = Header(buffer, offset);
            messageCount = readUintFromBuf(buffer, offset);
            for (int i = 0; i < messageCount; i++) {
                AppData data = AppData(buffer, offset);
                messages.push_back(data);
            }
        }

        std::string toString() {
            std::string out;
            out = "(Packet: " + header.toString();
            for (AppData message : messages) {
                out += ", " + message.toString();
            }
            out += ")";
            return out;
        }

        std::string toBuffer() {
            std::string buffer = header.toBuffer();
            writeUintToBuf(buffer, messageCount);
            for (AppData message : messages) {
                // copy the messageLength and the message into the buffer for
                // every message
                std::string m = message.toBuffer();
                buffer += m;
            }
            // std::cout << "Produced buffer: " << buffer << std::endl;
            return buffer;
        }
    };

    struct PacketHandled {
        Packet packet;
        std::chrono::high_resolution_clock::time_point sent;
        std::function<void(PacketHandled)> onResend;

        PacketHandled(Packet packet,
                      std::function<void(PacketHandled)> onResend)
            : packet(packet), onResend(onResend) {
            sent = std::chrono::high_resolution_clock::now();
        }
    };

  private:
    static void writeUintToBuf(std::string &buffer, uint data) {
        uint offset = buffer.length();
        buffer += std::string(sizeof(uint), ' ');
        std::memcpy(buffer.data() + offset, &data, sizeof(uint));
    }

    static uint readUintFromBuf(const std::string &buffer, uint &offset) {
        uint out;
        std::memcpy(&out, buffer.data() + offset, sizeof(uint));
        offset += sizeof(uint);
        return out;
    }
};
