file(GLOB src_files src/*.cpp)

add_executable(EmergenceClient ${src_files} ${api_sources})
target_link_libraries(EmergenceClient ncurses)
target_precompile_headers(EmergenceClient PUBLIC pch.hpp)
