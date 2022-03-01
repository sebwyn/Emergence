project(Emergence)

file(GLOB src_files src/*.cpp)

add_executable(EmergenceServer ${src_files} ${api_sources})
target_link_libraries(EmergenceServer ncurses)
target_precompile_headers(EmergenceServer PUBLIC pch.hpp)
