#pragma once

#include <ncurses.h>

#include "Utils.hpp"

typedef unsigned int uint;

// we're going to be streaming world data slightly larger than the view of
// the camera, so there is a little bit of lenience in the renderer
// TODO: make copy and read, take world frames not whole worlds
struct World {
    static const uint id = 2;

    uint width, height;
    char **data;
    World() : width(0), height(0), data(nullptr) {}
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
