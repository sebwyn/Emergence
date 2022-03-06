cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER clang++)

# set the project name
project(Emergence)

#add glfw here
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/glfw)

add_subdirectory(ext/glm)

include_directories(api api/core api/app ext/)
file(GLOB api_sources api/core/*.cpp api/app/*.cpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 17)

# add the executable
add_subdirectory(client)
add_subdirectory(server)
