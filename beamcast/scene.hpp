#pragma once

#include <filesystem>
#include <unordered_map>

#include <beamcast/data.hpp>
#include <camera.hpp>
#include <img/image.hpp>
#include <json/json.hpp>
#include <img/export.hpp>
#include <ranges>
#include <myglm/vec.h>
#include <materials.hpp>
#include "bvh.hpp"
#include "mesh.hpp"

class Scene {
   public:
	Camera camera;
	vec4   backgroundColor = {0.f, 0.f, 0.f, 1.f};

	struct ImageSettings {
		ivec2 resolution;

		ImageSettings() : resolution(0, 0) {}
		ImageSettings(const JSONObject &obj) {
			auto width	= obj["width"].as<JSONNumber>();
			auto height = obj["height"].as<JSONNumber>();
			resolution	= ivec2(width, height);
		}
	} imageSettings;

	std::vector<PointLight>					   lights;
	//std::vector<Mesh>						   objects;
	std::vector<std::unique_ptr<Material>>	   materials;
	std::vector<std::unique_ptr<Texture>>	   textures;
	std::unordered_map<std::string, Texture *> textureMap;

	std::filesystem::path scenePath;

	std::vector<Mesh> meshes;

	using MeshBVH = ygl::bvh::BVHTree<MeshObject*>;
	MeshBVH bvh;

	int frameCount = 1;

	const auto &getObjects() const { return bvh.getObjects(); }

	auto intersect(const Ray &r) const {
		RayHit hit;
		bvh.intersect(r, 0.0001f, FLT_MAX, hit);
		return hit;
	}

	auto fillHitInfo(RayHit &hit, const Ray &r, bool smooth = true) const {
		bvh.getObjects()[hit.objectIndex]->fillHitInfo(hit, r, smooth);
	}

	Scene()							= default;
	Scene(const Scene &)			= delete;
	Scene(Scene &&)					= delete;
	Scene &operator=(const Scene &) = delete;
	Scene &operator=(Scene &&)		= delete;

	Scene(const std::string_view &filename);

	void clear() {
		bvh.clear();
		lights.clear();
		camera = Camera{};
	}

	auto getImageSettings() const { return imageSettings; }

	void serializeOBJ(std::ostream &os) const {
		std::size_t offset = 0;
		for (const auto &object : bvh.getObjects()) {
			for (const auto &vertex : object->getVertices()) {
				os << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
			}
			for (const auto &normal : object->getNormals()) {
				os << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
			}
			for (const auto &index : object->getIndices()) {
				os << "f " << index.x + 1 + offset << "//" << index.x + 1 + offset << " " << index.y + 1 + offset
				   << "//" << index.y + 1 + offset << " " << index.z + 1 + offset << "//" << index.z + 1 + offset
				   << "\n";
			}
			offset += object->getVertices().size();
		}
	}

	Texture *getTexture(const std::string_view &name) const {
		auto it = textureMap.find(std::string(name));	  // TODO: use std::string_view for better performance
		if (it != textureMap.end()) { return it->second; }
		throw std::runtime_error(std::string("Texture not found: ") + std::string(name));
	}

	void setFrame(int frame) {
		camera.setFrame(frame);
	}
};
