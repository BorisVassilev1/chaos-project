cmake_minimum_required(VERSION 3.10)
project(BeamCast)

set(CMAKE_CXX_STANDARD 26)

add_subdirectory(myglm)
add_subdirectory(beamcast)
add_subdirectory(json)

include(CTest)
enable_testing()

add_custom_target(check
	COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	DEPENDS myglm_check
			json_check
)
