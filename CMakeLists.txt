cmake_minimum_required(VERSION 3.13)
project(sqlite-starter-cpp)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

# Reverted strictly back to "sqlite"
add_executable(sqlite ${SOURCE_FILES})