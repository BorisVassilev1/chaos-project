cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 26)

file(GLOB_RECURSE sources ./*.cpp ./*.c ../json/json.cpp ../img/*.cpp)

#message("Sources found: ${sources}")

add_executable(main ${sources})

target_include_directories(main PRIVATE ../lib ../ ../lib/sdp_2023/ )
target_include_directories(main PUBLIC .)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ../../)
target_link_libraries(main DPDA)
target_compile_options(main PRIVATE 
	-Wall -Wextra 
	-O3 
	-mavx -mavx2 -march=skylake 
	-g
	-fsanitize=address
	#-Rpass=loop-vectorize
	#-Rpass-missed=loop-vectorize
	#-Rpass-analysis=loop-vectorize
)
target_link_options(main PRIVATE 
	#-ltbb -ltbbmalloc -ltbbmalloc_proxy -pthread 
	-g -O3
	-mavx -mavx2 -march=native
	-fsanitize=address
)

