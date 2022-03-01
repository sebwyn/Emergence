#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <ncurses.h>
#include <optional>
#include <string>

#include "Utils.hpp"

#include "Protocol.hpp"

typedef unsigned int uint;

//Contains a couple of functions for reading and writing 
//arbitrary messages, as well as a get latest message
//function that iterates over a list of messages and gets
//the desired structs with the highest message IDs
class Messenger {
  public:
    template <class T>
    static std::optional<T> readMessage(const std::string &buffer) {
        uint offset = 0;
        uint id = Utils::readUintFromBuf(buffer, offset);
        std::string rest = buffer.substr(offset);
        if (id == T::id) {
            return T(rest);
        } else {
            return {};
        }
    }

    template <class... Types>
    static void getLatest(const std::vector<Protocol::AppData> &messages,
                          Types &...out) {
        std::map<uint, uint> highestIds;
        // initialize everything with zeros
        (
            [&](auto &input) {
                uint typeId = std::remove_reference<decltype(input)>::type::id;
                if (highestIds.find(typeId) != highestIds.end()) {
                    throw "Expected unique getLatest call";
                }
                highestIds[typeId] = 0;
            }(out),
            ...);
        for (auto message : messages) {
            if (message.message.length()) {
                (
                    [&](auto &input) {
                        using t = typename std::remove_reference<
                            decltype(input)>::type;
                        uint typeId = t::id;
                        auto castedType = readMessage<t>(message.message);
                        if (castedType.has_value()) {
                            if (message.id >= highestIds[typeId]) {
                                input = *castedType;
                            }
                        }
                    }(out),
                    ...);
            }
        }
    }

    template <class T> static std::string writeMessage(const T &object) {
        std::string buffer;
        Utils::writeUintToBuf(buffer, T::id);
        buffer += object.toBuffer();
        return buffer;
    }

  private:
};

struct PlayerInfo {
    static const uint id = 0;

    char symbol;
    PlayerInfo(char symbol) : symbol(symbol) {}
    PlayerInfo(const std::string &buffer) { symbol = buffer[0]; }

    std::string toBuffer() const { return std::string(1, symbol); }
    std::string toString() const {
        return "(PlayerInfo: (symbol: " + std::string(1, symbol) + "))";
    }
};

struct PlayerData {
    static const uint id = 1;

    int x, y;
    PlayerData(int x, int y) : x(x), y(y) {}
    PlayerData(const std::string &buffer) {
        uint offset = 0;
        x = Utils::readUintFromBuf(buffer, offset);
        y = Utils::readUintFromBuf(buffer, offset);
    }

    std::string toBuffer() const {
        std::string buffer;
        Utils::writeUintToBuf(buffer, x);
        Utils::writeUintToBuf(buffer, y);
        return buffer;
    }
    std::string toString() const {
        return "(PlayerData: (x: " + std::to_string(x) +
               ", y: " + std::to_string(y) + "))";
    }
};

// we're going to be streaming world data slightly larger than the view of
// the camera, so there is a little bit of lenience in the renderer
// TODO: make copy and read, take world frames not whole worlds
struct World {
    static const uint id = 2;

    uint width, height;
    char **data;
    World(uint width, uint height) : width(width), height(height) {
        data = new char *[height];
        for (uint y = 0; y < height; y++) {
            data[y] = new char[width + 1];
            // initialize with a bunch of periods
            for (uint x = 0; x < width; x++) {
                data[y][x] = '.';
            }
            data[y][width] = '\0';
        }
    }
    World(const std::string &buffer) {
        uint offset = 0;
        width = Utils::readUintFromBuf(buffer, offset);
        height = Utils::readUintFromBuf(buffer, offset);
        data = new char *[height];
        for (int y = 0; y < height; y++) {
            data[y] = new char[width + 1];
            std::memcpy(data[y], &(buffer.data()[offset + y * (width + 1)]),
                        width + 1);
        }
    }
    // copy constructor, necessary to deep copy pointers
    World(const World &world) {
        width = world.width;
        height = world.height;
        data = new char *[height];
        for (uint y = 0; y < height; y++) {
            data[y] = new char[width + 1];
            // initialize with a bunch of periods
            std::memcpy(data[y], world.data[y], (width + 1));
        }
    }
    void operator=(const World &world) {
        width = world.width;
        height = world.height;
        data = new char *[height];
        for (uint y = 0; y < height; y++) {
            data[y] = new char[width + 1];
            // initialize with a bunch of periods
            std::memcpy(data[y], world.data[y], (width + 1));
        }
    }
    ~World() {
        for (uint y = 0; y < height; y++) {
            delete data[y];
        }
        delete data;
    }
    std::string toBuffer() const {
        std::string buffer;
        Utils::writeUintToBuf(buffer, width);
        Utils::writeUintToBuf(buffer, height);
        for (uint y = 0; y < height; y++) {
            buffer += std::string(data[y], width + 1);
        }
        return buffer;
    }
    std::string toString() const {
        return "(World: (width: " + std::to_string(width) +
               ", height: " + std::to_string(height) + "))";
    }

    void draw() {
        for (int y = 0; y < height; y++) {
            mvaddstr(y, 0, data[y]);
        }
    }
};
