#include <scene.hpp>
#include "json/json.hpp"
#include "mesh.hpp"

Scene::Scene(const std::string_view &filename) {
	dbLog(dbg::LOG_DEBUG, "Loading scene from file: ", filename);
	this->scenePath = filename;
	try {
		auto json = JSONFromFile(filename);
		dbLog(dbg::LOG_DEBUG, "Parsed JSON from scene file: ", filename);
		if (json == nullptr) { throw std::runtime_error("Failed to load scene from file: " + std::string(filename)); }

		if (json->getType() != JSONType::Object) { throw std::runtime_error("Scene file must contain a JSON object"); }

		auto &jo	   = json->as<JSONObject>();
		auto &settings = jo["settings"].as<JSONObject>();

		auto &imageSettings = settings["image_settings"].as<JSONObject>();
		this->imageSettings = ImageSettings(imageSettings);

		auto &backgroundJSON  = settings["background_color"].as<JSONArray>();
		this->backgroundColor = vec4{backgroundJSON[0].as<JSONNumber>(), backgroundJSON[1].as<JSONNumber>(),
									 backgroundJSON[2].as<JSONNumber>(),
									 backgroundJSON.size() > 3 ? (float)backgroundJSON[3].as<JSONNumber>() : 1.0f};

		auto &cameraJSON = jo["camera"].as<JSONObject>();
		this->camera	 = Camera(cameraJSON);

		if(jo.find("meshes") != jo.end()) {
			const auto &meshesJSON = jo["meshes"].as<JSONArray>();
			for(const auto &j : meshesJSON) {
				auto &obj = j->as<JSONObject>();
				meshes.emplace_back(obj);
			}
		}

		auto &objectsJSON = jo["objects"].as<JSONArray>();
		for (const auto &j : objectsJSON) {
			const auto &obj = j->as<JSONObject>();
			if(obj.find("ref") == obj.end()) {
				meshes.emplace_back(obj);
				bvh.addPrimitive(new MeshObject(*this, meshes.size()-1, obj));
			} else {
				std::size_t meshIndex = obj["ref"].as<JSONNumber>();
				bvh.addPrimitive(new MeshObject(*this, meshIndex, obj));
			}
		}

		auto &lightsJSON = jo["lights"].as<JSONArray>();
		for (const auto &j : lightsJSON) {
			lights.emplace_back(j->as<JSONObject>());
		}

		if (jo.find("textures") != jo.end()) {
			auto &texturesJSON = jo["textures"].as<JSONArray>();
			dbLog(dbg::LOG_DEBUG, "Found ", texturesJSON.size(), " textures in scene file.");
			for (const auto &j : texturesJSON) {
				auto &obj = j->as<JSONObject>();
				if (obj["type"].as<JSONString>() == std::string_view("albedo")) {
					textures.emplace_back(new ConstantTexure(obj));
				} else if (obj["type"].as<JSONString>() == std::string_view("checker")) {
					textures.emplace_back(new CheckerTexture(obj));
				} else if (obj["type"].as<JSONString>() == std::string_view("edges")) {
					textures.emplace_back(new EdgeTexture(obj));
				} else if (obj["type"].as<JSONString>() == std::string_view("bitmap")) {
					textures.emplace_back(new ImageTexture(obj, this->scenePath));
				} else {
					throw std::runtime_error("Unknown texture type: " + std::string(obj["type"].as<JSONString>()));
				}
				textureMap[obj["name"].as<JSONString>()] = textures.back().get();
			}
		}

		materials.emplace_back(std::make_unique<DiffuseMaterial>(vec3(1.0)));
		materials.back()->smooth = false;
		if (jo.find("materials") == jo.end()) {
			dbLog(dbg::LOG_WARNING, "No materials found in scene file, using default materials.");
		} else {
			auto &materialsJSON = jo["materials"].as<JSONArray>();
			for (const auto &j : materialsJSON) {
				auto &obj = j->as<JSONObject>();
				if (obj["type"].as<JSONString>() == std::string_view("diffuse")) {
					materials.emplace_back(std::make_unique<DiffuseMaterial>(obj, *this));
				} else if (obj["type"].as<JSONString>() == std::string_view("reflective")) {
					materials.emplace_back(std::make_unique<ReflectiveMaterial>(obj));
				} else if (obj["type"].as<JSONString>() == std::string_view("refractive")) {
					materials.emplace_back(std::make_unique<RefractiveMaterial>(obj));
				} else if (obj["type"].as<JSONString>() == std::string_view("constant")) {
					materials.emplace_back(std::make_unique<ConstantMaterial>(obj));
				} else {
					throw std::runtime_error("Unknown material type: " + std::string(obj["type"].as<JSONString>()));
				}
			}
		}

		bvh.build();
		dbLog(dbg::LOG_DEBUG, "Scene loaded with ", getObjects().size(), " objects, ", lights.size(), " lights, and ",
			  materials.size(), " materials.");
	} catch (const std::exception &e) {
		clear();
		throw;
	}
}
