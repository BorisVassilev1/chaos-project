cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 26)

file(GLOB_RECURSE sources ./*.cpp ./*.c)

add_executable(main ${sources})

target_include_directories(main PRIVATE ../lib ../)
target_include_directories(main PUBLIC .)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ../../)

