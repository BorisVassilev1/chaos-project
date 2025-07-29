#pragma once

#include <data.hpp>
#include <filesystem>
#include <img/image.hpp>

class Texture {
   public:
	virtual ~Texture() = default;

	virtual vec3 sample(const RayHit &hit) const = 0;
};

class ConstantTexure : public Texture {
   public:
	vec3 albedo;

	ConstantTexure(const vec3 &albedo) : albedo(albedo) {}
	ConstantTexure(const JSONObject &obj) {
		const auto &albedoJSON = obj["albedo"].as<JSONArray>();
		albedo = vec3{albedoJSON[0].as<JSONNumber>(), albedoJSON[1].as<JSONNumber>(), albedoJSON[2].as<JSONNumber>()};
	}

	vec3 sample(const RayHit &) const override { return albedo; }
};

class CheckerTexture : public Texture {
   public:
	vec3  color1;
	vec3  color2;
	float scale;

	CheckerTexture(const vec3 &color1, const vec3 &color2, float scale)
		: color1(color1), color2(color2), scale(scale) {}

	CheckerTexture(const JSONObject &obj) {
		const auto &color1JSON = obj["color_A"].as<JSONArray>();
		const auto &color2JSON = obj["color_B"].as<JSONArray>();
		color1 = vec3{color1JSON[0].as<JSONNumber>(), color1JSON[1].as<JSONNumber>(), color1JSON[2].as<JSONNumber>()};
		color2 = vec3{color2JSON[0].as<JSONNumber>(), color2JSON[1].as<JSONNumber>(), color2JSON[2].as<JSONNumber>()};
		scale  = obj["square_size"].as<JSONNumber>();
	}

	vec3 sample(const RayHit &hit) const override {
		auto checker = int(hit.texCoords.x / scale) % 2 == int(hit.texCoords.y / scale) % 2;
		return checker ? color1 : color2;
	}
};

class EdgeTexture : public Texture {
   public:
	vec3  color1;
	vec3  color2;
	float edgeWidth;

	EdgeTexture(const vec3 &color1, const vec3 &color2, float edgeWidth)
		: color1(color1), color2(color2), edgeWidth(edgeWidth) {}

	EdgeTexture(const JSONObject &obj) {
		const auto &color1JSON = obj["edge_color"].as<JSONArray>();
		const auto &color2JSON = obj["inner_color"].as<JSONArray>();
		color1 = vec3{color1JSON[0].as<JSONNumber>(), color1JSON[1].as<JSONNumber>(), color1JSON[2].as<JSONNumber>()};
		color2 = vec3{color2JSON[0].as<JSONNumber>(), color2JSON[1].as<JSONNumber>(), color2JSON[2].as<JSONNumber>()};
		edgeWidth = obj["edge_width"].as<JSONNumber>();
	}

	vec3 sample(const RayHit &hit) const override {
		float dist = std::min(hit.uv.x, std::min(hit.uv.y, 1.0f - hit.uv.x - hit.uv.y));
		if (dist < edgeWidth) {
			return color1;
		} else {
			return color2;
		}
	}
};

class ImageTexture : public Texture {
   public:
	Image<RGB32F> image;

	ImageTexture(const JSONObject &obj, const std::filesystem::path &scenePath) {
		const auto &filename	= obj["file_path"].as<JSONString>();
		auto		sceneFolder = scenePath.parent_path();
		if (sceneFolder.empty()) { sceneFolder = std::filesystem::current_path(); }
		auto fullPath = std::filesystem::path(std::string_view(filename));
		if (fullPath.is_relative()) fullPath = sceneFolder.concat(fullPath.string());

		image.loadFromFile(fullPath);
	}

	vec3 sample(const RayHit &hit) const override {
		return image.sample(hit.texCoords.xy());
	}
};
