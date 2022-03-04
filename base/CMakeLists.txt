# CMakeList.txt : CMake project for biggy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.

add_executable (biggy "biggy.cpp" "biggy.h")
target_include_directories(biggy PUBLIC $<BUILD_INTERFACE:${bigbackend_SOURCE_DIR}/include>)
target_link_libraries(biggy backend)


# TODO: Add tests and install targets if needed.
