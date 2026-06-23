cmake_minimum_required(VERSION 3.13)
project(sqlite-starter-cpp)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

# Change the target name to "server" so it aligns with CodeCrafters' run scripts
add_executable(server ${SOURCE_FILES})