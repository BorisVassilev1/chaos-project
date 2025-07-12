#include <iostream>

#include <img/export.hpp>

#include <camera.hpp>
#include <scene.hpp>

int main(int argc, char** argv) {
	if (argc <= 1) {
		std::cerr << "Usage: " << argv[0] << " <scene_file>" << std::endl;
		return 1;
	}

	std::cout << "Loading scene from file: " << argv[1] << std::endl;
	Scene sc(argv[1]);
	std::cout << "Parsed Scene: " << argv[1] << std::endl;

	if (argc > 2) {
		try {
			float scale = std::stof(argv[2]);
			sc.setResolutionScale(scale);
		} catch (const std::exception& e) {
			std::cerr << "Invalid resolution scale: " << e.what() << std::endl;
			return 1;
		}
	}

	sc.render();
	sc.saveImage("output.ppm");

	// system("fish -c 'open output.ppm'");
}
