cmake_minimum_required(VERSION 3.10)
project(BeamCast)

set(CMAKE_CXX_STANDARD 26)

add_subdirectory(myglm)
add_subdirectory(beamcast)

include(CTest)
enable_testing()
