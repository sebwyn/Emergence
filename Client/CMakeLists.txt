
file(GLOB src_files src/*.cpp ${PROJECT_SOURCE_DIR}/api/*.cpp)

add_executable(EmergenceClient ${src_files})
target_link_libraries(EmergenceClient ncurses)
