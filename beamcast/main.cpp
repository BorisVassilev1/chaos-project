#include <iostream>

#include <img/export.hpp>

#include <camera.hpp>
#include <scene.hpp>
#include "renderer.hpp"
#include <fenv.h>

/*

timings				for 1 SPP on the dragon scene from exercise 13:
single thread:		315556 ms
par_unseq:			56904 ms
par:				57249 ms
sectors:			56298 ms
bvh:				183ms
*/

int main(int argc, char** argv) {
	// feenableexcept(FE_INVALID);
	if (argc <= 1) {
		dbLog(dbg::LOG_ERROR, "No scene file provided.");
		dbLog(dbg::LOG_ERROR, "Usage: ", argv[0], " <scene_file> [resolution_scale] [samples_per_pixel] [a: render entire animation] [num_threads]");
		return 1;
	}

	std::unique_ptr<Scene> sc;
	try {
		sc = std::make_unique<Scene>(argv[1]);
	} catch (const std::exception& e) {
		dbLog(dbg::LOG_ERROR, "Failed to load scene: ", e.what());
		return 1;
	}

	float resolution_scale = 1.0f;
	if (argc > 2) {
		if (argv[2] == std::string("-")) {
			std::ofstream ofs("output.obj");
			sc->serializeOBJ(ofs);
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

	int spp = 1;
	if(argc > 3) {
		try {
			spp = std::stoi(argv[3]);
		} catch (const std::exception& e) {
			dbLog(dbg::LOG_ERROR, "Invalid samples per pixel: ", e.what());
			return 1;
		}
	}

	int entire_animation = false;
	if(argc > 4 && argv[4][0] == 'a') {
		entire_animation = true;
	}

	int threadCount = std::thread::hardware_concurrency();
	if (argc > 5) {
		try {
			threadCount = std::stoi(argv[5]);
		} catch (const std::exception& e) {
			dbLog(dbg::LOG_ERROR, "Invalid thread count: ", e.what());
			return 1;
		}
	}

	Renderer rend(*sc, resolution_scale, threadCount, spp);
	dbLog(dbg::LOG_INFO, "Starting animation render with ", sc->frameCount, "frames");
	for (int i = 0; i < (entire_animation ? sc->frameCount : 1); ++i) {
		if(entire_animation) sc->setFrame(i);
		auto start = std::chrono::high_resolution_clock::now();
		rend.render();
		auto end	  = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		dbLog(dbg::LOG_INFO, "Rendering completed in ", duration.count(), " ms");
		rend.saveImage(std::format("output_{:0>3}.png", i));
	}

	system("fish -c 'feh output_0.png'");
}
