
file(GLOB src_files src/*.cpp ${PROJECT_SOURCE_DIR}/api/*.cpp)

add_executable(EmergenceServer ${src_files})