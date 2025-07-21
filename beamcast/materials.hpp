#pragma once

#include <data.hpp>
#include "textures.hpp"

class Scene;

class Material {
   public:
	bool smooth : 1			 = false;
	bool castsShadows : 1	 = true;
	bool receivesShadows : 1 = true;

	Material() = default;
	Material(const JSONObject &obj, bool castsShadows = true, bool receivesShadows = true)
		: castsShadows(castsShadows), receivesShadows(receivesShadows) {
		if (obj.find("smooth_shading") != obj.end()) { smooth = obj["smooth_shading"].as<JSONBoolean>(); }
	}

	virtual vec4 shade(const RayHit &hit, const Ray &ray, const Scene &scene) const = 0;

	virtual ~Material() = default;
};

class DiffuseMaterial : public Material {
   public:
	Texture *albedo;
	vec3  albedoColor;

	DiffuseMaterial(const vec3 &albedo) : albedo(nullptr), albedoColor(albedo) {}

	DiffuseMaterial(const JSONObject &obj, const Scene &scene);
	vec4 shade(const RayHit &hit, const Ray &, const Scene &scene) const override;
};

class ReflectiveMaterial : public Material {
   public:
	vec3 albedo;

	ReflectiveMaterial(const vec3 &albedo) : albedo(albedo) {}

	ReflectiveMaterial(const JSONObject &obj) : Material(obj, true, true) {
		const auto &colorJSON = obj["albedo"].as<JSONArray>();
		albedo = vec3{colorJSON[0].as<JSONNumber>(), colorJSON[1].as<JSONNumber>(), colorJSON[2].as<JSONNumber>()};
	}

	vec4 shade(const RayHit &hit, const Ray &, const Scene &scene) const override;
};

class RefractiveMaterial : public Material {
   public:
	vec3  albedo;
	float ior;

	RefractiveMaterial(const vec3 &albedo) : albedo(albedo) {}

	RefractiveMaterial(const JSONObject &obj) : Material(obj, false, false) {
		if (obj.find("ior") != obj.end()) {
			ior = obj["ior"].as<JSONNumber>();
		} else {
			ior = 1.5f;		// Default IOR for glass
		}
		if (obj.find("albedo") != obj.end()) {
			const auto &colorJSON = obj["albedo"].as<JSONArray>();
			albedo = vec3{colorJSON[0].as<JSONNumber>(), colorJSON[1].as<JSONNumber>(), colorJSON[2].as<JSONNumber>()};
		} else {
			albedo = vec3(1.0f, 1.0f, 1.0f);	 // Default albedo
		}
	}

	vec4 shade(const RayHit &hit, const Ray &, const Scene &scene) const override;
};

class ConstantMaterial : public Material {
   public:
	vec3 albedo;

	ConstantMaterial(const vec3 &albedo) : albedo(albedo) {}

	ConstantMaterial(const JSONObject &obj) : Material(obj, true, false) {
		const auto &albedoJSON = obj["albedo"].as<JSONArray>();
		albedo = vec3{albedoJSON[0].as<JSONNumber>(), albedoJSON[1].as<JSONNumber>(), albedoJSON[2].as<JSONNumber>()};
	}

	vec4 shade(const RayHit &, const Ray &, const Scene &) const override { return vec4(albedo, 1.0f); }
};
