#pragma once

#include <beamcast/data.hpp>
#include <limits>
#include <camera.hpp>
#include <img/image.hpp>
#include <json/json.hpp>
#include <img/export.hpp>
#include <ranges>

class Scene {
   public:
	Camera		  camera;
	Image<RGB32F> image;
	vec4		  backgroundColor = {0.f, 0.f, 0.f, 1.f};

	ivec2 resolution;
	float resolution_scale = 1.0f;

	std::vector<Triangle> triangles;

	auto intersect(const Ray &r) const {
		float t	  = std::numeric_limits<float>::max();
		int	  tid = -1;
		for (const auto &[i, triangle] : std::views::enumerate(triangles)) {
			auto tnew = triangle.intersect(r);
			if (tnew > 0.f && tnew < t) { t = tnew; tid = i;}
		}
		return RayHit(t, tid);
	}

	Scene()							= default;
	Scene(const Scene &)			= default;
	Scene(Scene &&)					= default;
	Scene &operator=(const Scene &) = default;
	Scene &operator=(Scene &&)		= default;
	~Scene()						= default;

	Scene(const std::string_view &filename) {
		try {
			auto json = JSONFromFile(filename);
			if (json == nullptr) {
				throw std::runtime_error("Failed to load scene from file: " + std::string(filename));
			}

			if (json->getType() != JSONType::Object) {
				throw std::runtime_error("Scene file must contain a JSON object");
			}

			auto &jo	   = json->as<JSONObject>();
			auto &settings = jo["settings"].as<JSONObject>();

			auto &imageSettings = settings["image_settings"].as<JSONObject>();
			this->image			= Image<RGB32F>(imageSettings);
			this->resolution	= image.resolution();

			auto &backgroundJSON  = settings["background_color"].as<JSONArray>();
			this->backgroundColor = vec4{backgroundJSON[0].as<JSONNumber>(), backgroundJSON[1].as<JSONNumber>(),
										 backgroundJSON[2].as<JSONNumber>(),
										 backgroundJSON.size() > 3 ? (float)backgroundJSON[3].as<JSONNumber>() : 1.0f};

			auto &cameraJSON = jo["camera"].as<JSONObject>();
			this->camera	 = Camera(cameraJSON);

			auto			 &objects = jo["objects"].as<JSONArray>();
			std::vector<vec3> vertexArray;
			for (const auto &j : objects) {
				auto &obj	   = j->as<JSONObject>();
				auto &vertices = obj["vertices"].as<JSONArray>();
				for (unsigned int i = 0; i < vertices.size(); i += 3) {
					if (i + 2 >= vertices.size()) {
						throw std::runtime_error("Invalid number of vertices in triangle object");
					}
					vec3 v0 = {vertices[i].as<JSONNumber>(), vertices[i + 1].as<JSONNumber>(),
							   vertices[i + 2].as<JSONNumber>()};
					vertexArray.push_back(v0);
				}
				auto &indices = obj["triangles"].as<JSONArray>();
				if (indices.size() % 3 != 0) {
					throw std::runtime_error("Indices must be a multiple of 3 for triangle objects");
				}
				for (unsigned int i = 0; i < indices.size(); i += 3) {
					if (i + 2 >= indices.size()) {
						throw std::runtime_error("Invalid number of indices in triangle object");
					}
					unsigned int idx0 = indices[i].as<JSONNumber>();
					unsigned int idx1 = indices[i + 1].as<JSONNumber>();
					unsigned int idx2 = indices[i + 2].as<JSONNumber>();

					if (idx0 >= vertexArray.size() || idx1 >= vertexArray.size() || idx2 >= vertexArray.size()) {
						throw std::runtime_error("Index out of bounds in triangle object");
					}

					triangles.emplace_back(vertexArray[idx0], vertexArray[idx1], vertexArray[idx2]);
				}
			}

		} catch (const std::exception &e) {
			std::cerr << "Error loading scene: " << e.what() << std::endl;
			clear();
		}
	}

	void clear() {
		triangles.clear();
		camera = Camera{};
	}

	void render() {
		RGB32F colors[] = {vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f),
						 vec3(1.f, 1.f, 0.f), vec3(0.f, 1.f, 1.f), vec3(1.f, 0.f, 1.f)};

		PercentLogger logger("Rendering", image.getWidth() * image.getHeight());
		for (auto [x, y] : image.Iterate()) {
			logger.step();

			auto r		  = camera.generate_ray(ivec2(x, y), image.resolution());
			auto [t, tid] = intersect(r);

			if (t == std::numeric_limits<float>::max()) {
				image(x, y) = backgroundColor.xyz();
				continue;
			}

			RGB32F color = colors[tid % 6];
			image(x, y)	 = color.xyz();
		}
		logger.finish();
	};

	void saveImage(const std::string_view &filename) const {
		exportToFile(image, filename, export_PPM_BIN<RGB32F>);
		std::cout << "Image saved to " << filename << std::endl;
	}

	void setResolutionScale(float scale) {
		if (scale <= 0.f) { throw std::invalid_argument("Resolution scale must be greater than 0"); }
		resolution_scale = scale;
		image.resize(resolution.x * scale, resolution.y * scale);
	}
};
