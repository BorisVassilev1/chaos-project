#pragma once

#include <threading.hpp>
#include <img/image.hpp>
#include <data.hpp>
#include <scene.hpp>
#include <log.hpp>
#include "sample.hpp"

class Renderer {
	OneShotThreadPool pool;
	Image<RGBA32F>	  image;
	float			  resolution_scale = 1.0f;
	int spp;

	Scene &scene;

   public:
	Renderer(Scene &scene, float resolution_scale = 1.0f, int threadCount = std::thread::hardware_concurrency(), int spp = 1)
		: pool(threadCount), resolution_scale(resolution_scale), spp(spp), scene(scene) {
		setResolutionScale(resolution_scale);
	}

	void setResolutionScale(float scale) {
		if (scale <= 0.f) { throw std::runtime_error("Resolution scale must be greater than 0"); }
		resolution_scale		  = scale;
		const auto &imageSettings = scene.imageSettings;
		image.resize(imageSettings.resolution.x * resolution_scale, imageSettings.resolution.y * resolution_scale);
	}

	void render() {
		auto		  I = segmentImage(image.resolution(), ivec2(32, 32));
		PercentLogger logger("Rendering", I.size());
		scene.camera.setResolution(image.resolution());
		pool.reset();

		auto f = [&](const std::any &job) {
			auto segment = std::any_cast<std::pair<ivec2, ivec2>>(job);
			uint32_t seed = rand();
			for (const auto &coord : iter2D(segment.first, segment.second)) {
				RGBA32F color = 0;
				for (int i = 0; i < spp; ++i) {
					color += shadePixel(coord, seed);
				}
				color /= (float)spp;	 // Average over 10 samples
				color					= clamp(color, 0.f, 1.f);
				image(coord.x, coord.y) = color;
			}
			logger.step();
		};

		for (const auto &segment : I) {
			pool.addJob(std::any(segment), f);
		}
		pool.start();
		pool.wait();

		logger.finish();
	};

	void saveImage(const std::string_view &filename) const {
		exportToFile<export_PNG>(image, filename);
		dbLog(dbg::LOG_INFO, "Image saved to ", filename, "\n");
	}

	RGBA32F shadePixel(const ivec2 &pixel, uint32_t &seed) const {
		const auto &x	= pixel.x;
		const auto &y	= pixel.y;
		auto		r	= scene.camera.generate_ray(ivec2(x, y), seed);
		auto		hit = scene.intersect(r);
		if (hit.t == std::numeric_limits<float>::max()) { return scene.backgroundColor; }

		const auto &object		  = scene.getObjects()[hit.objectIndex];
		auto		materialIndex = object->getMaterialIndex();
		const auto &material	  = scene.materials[materialIndex];
		scene.fillHitInfo(hit, r, material->smooth);

		seed = pcg_hash(x + y * image.resolution().x + seed);

		vec4 color = material->shade(hit, r, scene, seed);

		return color;
	}
};
