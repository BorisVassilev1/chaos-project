#include <iostream>

#include <img/export.hpp>

#include <camera.hpp>
#include <scene.hpp>

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

	if (argc > 2) {
		if (argv[2] == std::string("-")) {
			std::ofstream ofs("output.obj");
			sc.serializeOBJ(ofs);
			dbLog(dbg::LOG_INFO, "Scene exported to output.obj");
			return 0;
		}
		try {
			float scale = std::stof(argv[2]);
			sc.setResolutionScale(scale);
		} catch (const std::exception& e) {
			dbLog(dbg::LOG_ERROR, "Invalid resolution scale: ", e.what());
			return 1;
		}
	}

	sc.render();
	sc.saveImage("output.ppm");

	system("fish -c 'open output.ppm'");
}
