#include <scene.hpp>

Scene::Scene(const std::string_view &filename) {
	dbLog(dbg::LOG_DEBUG, "Loading scene from file: ", filename);
	try {
		auto json = JSONFromFile(filename);
		if (json == nullptr) { throw std::runtime_error("Failed to load scene from file: " + std::string(filename)); }

		if (json->getType() != JSONType::Object) { throw std::runtime_error("Scene file must contain a JSON object"); }

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

		auto &objectsJSON = jo["objects"].as<JSONArray>();
		for (const auto &j : objectsJSON) {
			auto &obj = j->as<JSONObject>();
			objects.emplace_back(obj);
		}

		auto &lightsJSON = jo["lights"].as<JSONArray>();
		for (const auto &j : lightsJSON) {
			lights.emplace_back(j->as<JSONObject>());
		}

		auto &materialsJSON = jo["materials"].as<JSONArray>();
		for (const auto &j : materialsJSON) {
			auto &obj = j->as<JSONObject>();
			if (obj["type"].as<JSONString>() == std::string_view("diffuse")) {
				materials.emplace_back(std::make_unique<DiffuseMaterial>(obj));
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
		dbLog(dbg::LOG_DEBUG, "Scene loaded with ", objects.size(), " objects, ", lights.size(), " lights, and ",
			  materials.size(), " materials.");
	} catch (const std::exception &e) {
		clear();
		throw;
	}
}

RGB32F Scene::shadePixel(const ivec2 &pixel) const {
	const auto &x	= pixel.x;
	const auto &y	= pixel.y;
	auto		r	= camera.generate_ray(ivec2(x, y), image.resolution());
	auto		hit = intersect(r);
	if (hit.t == std::numeric_limits<float>::max()) { return backgroundColor.xyz(); }

	const auto &object		  = objects[hit.objectIndex];
	auto		materialIndex = object.getMaterialIndex();
	const auto &material	  = materials[materialIndex];
	fillHitInfo(hit, r, material->smooth);

	vec4 color = material->shade(hit, r, *this);

	return color.xyz();
}
