﻿# CMakeList.txt : CMake project for biggy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.9)

# Add source to this project's executable.

add_executable (biggy "biggy.cpp" "biggy.h")

target_link_libraries(biggy "backend")

# TODO: Add tests and install targets if needed.
