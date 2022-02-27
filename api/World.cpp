#include "World.hpp"

#include <cstring>

World::World(uint width, uint height) : width(width), height(height) {
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

World::~World() {
    for (uint y = 0; y < height; y++) {
        delete data[y];
    }
}

std::string World::toBuffer() {
    std::string buffer;

    for (uint y = 0; y < height; y++) {
        buffer += std::string(data[height], width + 1);
    }

    return buffer;
}

void World::fromBuffer(std::string &buffer) {
    uint offset = 0;
    for (int y = 0; y < height; y++) {
        std::memcpy(data[y], &(buffer.data()[y * width + 1]), width + 1);
    }
}

void World::draw() {
    for (int y = 0; y < height; y++) {
        mvaddstr(y, 0, data[y]);
    }
}
