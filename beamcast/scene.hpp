#pragma once

#include <algorithm>
#include <filesystem>
#include <unordered_map>

#include <beamcast/data.hpp>
#include <execution>
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
	std::vector<Texture *>			   textures;
	std::unordered_map<std::string, Texture *> textureMap;

	std::filesystem::path scenePath;

	auto intersect(const Ray &r) const {
		RayHit hit;
		for (const auto &[i, object] : std::views::enumerate(objects)) {
			const auto &material = materials[object.getMaterialIndex()];
			if (r.type == Ray::Type::Shadow && !material->castsShadows) continue;
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
		camera.setResolution(image.resolution());
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

	void serializeOBJ(std::ostream &os) const {
		std::size_t offset = 0;
		for (const auto &object : objects) {
			for (const auto &vertex : object.getVertices()) {
				os << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
			}
			for (const auto &normal : object.getNormals()) {
				os << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
			}
			for (const auto &index : object.getIndices()) {
				os << "f " << index.x + 1 + offset << "//" << index.x + 1 + offset << " " << index.y + 1 + offset
				   << "//" << index.y + 1 + offset << " " << index.z + 1 + offset << "//" << index.z + 1 + offset
				   << "\n";
			}
			offset += object.getVertices().size();
		}
	}

	Texture *getTexture(const std::string_view &name) const {
		auto it = textureMap.find(std::string(name)); // TODO: use std::string_view for better performance
		if (it != textureMap.end()) {
			return it->second;
		}
		throw std::runtime_error(std::string("Texture not found: ") + std::string(name));
	}
};
