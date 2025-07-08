cmake_minimum_required(VERSION 3.10)
project(myglm)

file(GLOB_RECURSE sources ./*.cpp ./*.c)
file(GLOB_RECURSE sources_test test.cpp)

add_executable(myglm_test ${sources_test})

#target_link_libraries(myglm_test myglm)
target_include_directories(myglm_test PRIVATE ../lib ..)

include(CTest)
enable_testing()
add_test(NAME myglmTests
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/myglm_test
)

add_custom_target(myglm_check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	DEPENDS myglm_test
)



