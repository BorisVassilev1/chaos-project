#include <iostream>

#include <img/export.hpp>

#include <fstream>
#include "camera.hpp"
#include "scene.hpp"

int main() { 

	Image<RGB> image(960,540);

	Scene sc;

	Camera camera{
		mat4(1.0f),
		45.0f,
		image.resolution()
	};

	for(auto [x, y] : image.Iterate()) {
		auto ray = camera.generate_ray(ivec2(x, y));
		//image(x, y) = convert<RGB32F, RGB>((ray.direction + vec3( 1., 1., 0.)) * 0.5f);

		auto r = camera.generate_ray(ivec2(x, y));
		auto t = sc.intersect(r);
		t = std::clamp(t, 0.f, 1.f);

		RGB32F color = {t,t,t};
		image(x,y) = convert<RGB32F, RGB>(color);

	}



	std::ofstream out("output.ppm");
	export_PPM_BIN(image, out);


	system("fish -c 'open output.ppm'");

}
