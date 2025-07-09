cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE sources_test test.cpp json.cpp)

add_subdirectory(../lib/sdp_2023/ sdp_2023)

add_executable(json_test ${sources_test})
target_include_directories(json_test PRIVATE ../lib .. ../lib/sdp_2023/)
target_link_libraries(json_test DPDA)

include(CTest)
enable_testing()
add_test(NAME jsonTests
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/json_test
)

add_custom_target(json_check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	DEPENDS json_test
)

