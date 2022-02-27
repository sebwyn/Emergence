#pragma once 

#include <ncurses.h>
#include <string>
#include <memory>


typedef unsigned int uint;

struct World {
    uint width, height;
    char** data;

    World(uint width, uint height);
    ~World();
    
    //we're going to be streaming world data slightly larger than the view of the
    //camera, so there is a little bit of lenience in the renderer
    //TODO: make copy and read, take world frames not whole worlds

    //server-side programs
    std::string toBuffer();
    //construct delta (problematic because packets can get dropped)
    //std::vector<std::string> constructDelta(World world);

    //client-side program
    void fromBuffer(std::string& buffer);

    void draw();
};
