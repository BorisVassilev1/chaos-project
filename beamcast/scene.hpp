#pragma once

#include <algorithm>
#include <beamcast/data.hpp>
#include <execution>
#include <limits>
#include <camera.hpp>
#include <img/image.hpp>
#include <json/json.hpp>
#include <img/export.hpp>
#include <ranges>
#include <myglm/vec.h>
#include <materials.hpp>

class Scene {
   public:
	Camera		  camera;
	Image<RGB32F> image;
	vec4		  backgroundColor = {0.f, 0.f, 0.f, 1.f};

	ivec2 resolution;
	float resolution_scale = 1.0f;

	std::vector<PointLight>				   lights;
	std::vector<Mesh>					   objects;
	std::vector<std::unique_ptr<Material>> materials;

	auto intersect(const Ray &r) const {
		RayHit hit;
		for (const auto &[i, object] : std::views::enumerate(objects)) {
			auto hitnew = object.intersect(r);
			if (hitnew.t > 0.0001f && hitnew.t < hit.t) {
				hit				= hitnew;
				hit.objectIndex = i;
			}
		}
		return hit;
	}

	auto fillHitInfo(RayHit &hit, const Ray &r, bool smooth = true) const {
		objects[hit.objectIndex].fillHitInfo(hit, r, smooth);
	}

	Scene()							= default;
	Scene(const Scene &)			= default;
	Scene(Scene &&)					= default;
	Scene &operator=(const Scene &) = default;
	Scene &operator=(Scene &&)		= default;
	~Scene()						= default;

	Scene(const std::string_view &filename);

	void clear() {
		objects.clear();
		lights.clear();
		image.clear();
		camera = Camera{};
	}

	RGB32F shadePixel(const ivec2 &pixel) const;

	void render() {
		PercentLogger logger("Rendering", image.getWidth() * image.getHeight());
		// for (auto [x, y] : image.Iterate()) {
		auto I = image.Iterate();
		std::for_each(std::execution::par_unseq, I.begin(), I.end(), [&](const auto &pair) {
			// std::for_each(I.begin(), I.end(), [&](const auto &pair) {
			auto [x, y] = pair;

			auto color = shadePixel(ivec2(x, y));

			apply_inplace(color, [](float &c) { return std::clamp(c, 0.f, 1.f); });
			image(x, y) = color.xyz();
			logger.step();
		});
		logger.finish();
	};

	void saveImage(const std::string_view &filename) const {
		exportToFile(image, filename, export_PPM_BIN<RGB32F>);
		dbLog(dbg::LOG_INFO, "Image saved to ", filename, "\n");
	}

	void setResolutionScale(float scale) {
		if (scale <= 0.f) { throw std::invalid_argument("Resolution scale must be greater than 0"); }
		resolution_scale = scale;
		image.resize(resolution.x * scale, resolution.y * scale);
	}
};
