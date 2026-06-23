cmake_minimum_required(VERSION 3.10)

# FORCE CMAKE TO USE W64DEVKIT BEFORE IT LOOKS AT S_PROJECT SYSTEM CACHE
set(CMAKE_C_COMPILER "D:/w64devkit/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "D:/w64devkit/bin/g++.exe")

project(MyProjectName)