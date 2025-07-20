#include <iostream>

#include <img/export.hpp>

#include <camera.hpp>
#include <scene.hpp>
#include "renderer.hpp"

/*

timings:
single thread:		315556 ms
par_unseq:			56904 ms
par:				57249 ms
sectors:			56298 ms
bvh:				183ms
*/

int main(int argc, char** argv) {
	if (argc <= 1) {
		dbLog(dbg::LOG_ERROR, "No scene file provided.");
		dbLog(dbg::LOG_ERROR, "Usage: ", argv[0], " <scene_file> [resolution_scale]");
		return 1;
	}

	Scene sc;
	try {
		sc = Scene(argv[1]);
	} catch (const std::exception& e) {
		dbLog(dbg::LOG_ERROR, "Failed to load scene: ", e.what());
		return 1;
	}

	float resolution_scale = 1.0f;
	if (argc > 2) {
		if (argv[2] == std::string("-")) {
			std::ofstream ofs("output.obj");
			sc.serializeOBJ(ofs);
			dbLog(dbg::LOG_INFO, "Scene exported to output.obj");
			return 0;
		}
		try {
			resolution_scale = std::stof(argv[2]);
		} catch (const std::exception& e) {
			dbLog(dbg::LOG_ERROR, "Invalid resolution scale: ", e.what());
			return 1;
		}
	}

	int threadCount = std::thread::hardware_concurrency();
	if(argc > 3) {
		try{
			threadCount = std::stoi(argv[3]);
		} catch (const std::exception& e) {
			dbLog(dbg::LOG_ERROR, "Invalid thread count: ", e.what());
			return 1;
		}
	}

	Renderer rend(sc, resolution_scale, threadCount);
	auto start = std::chrono::high_resolution_clock::now();
	rend.render();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	dbLog(dbg::LOG_INFO, "Rendering completed in ", duration.count(), " ms");

	rend.saveImage("output.png");

	system("fish -c 'feh output.png'");
}
