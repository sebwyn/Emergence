cmake_minimum_required(VERSION 3.10)

# set the project name
project(Emergence)

include_directories(api)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 20)

# add the executable
add_subdirectory(Client)
add_subdirectory(Server)