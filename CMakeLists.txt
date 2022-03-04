# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.9)
project ("bigbackend")




# Include sub-projects.

add_subdirectory ("src")
add_subdirectory ("base")

set_property(TARGET backend PROPERTY CXX_STANDARD 17)

include(CTest)
enable_testing()
add_subdirectory("tests")