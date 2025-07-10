#include <scene.hpp>

Scene::Scene(const std::string_view &filename) {
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

	} catch (const std::exception &e) {
		std::cerr << "Error loading scene: " << e.what() << std::endl;
		clear();
	}
}

RGB32F Scene::shadePixel(const ivec2 &pixel) const {
	const auto &x	= pixel.x;
	const auto &y	= pixel.y;
	auto		r	= camera.generate_ray(ivec2(x, y), image.resolution());
	auto		hit = intersect(r);
	if (hit.t == std::numeric_limits<float>::max()) { return backgroundColor.xyz(); }
	fillHitInfo(hit, r);

	vec3 normal = hit.normal;
	vec3 color	= vec3(0.f, 0.f, 0.f);
	for (const auto &light : lights) {
		vec3  lightDir = light.position - hit.pos;
		float distance = length(lightDir);

		auto shadowHit = intersect(Ray(hit.pos + normal * 0.001f, lightDir));
		if (shadowHit.t > 0.f && shadowHit.t < distance) {
			continue;	  // shadow
		}

		color += light.color * light.intensity * std::max(0.f, dot(normal, lightDir)) /
				 (4.f * M_PIf * distance * distance * distance);
	}
	//color = normal;

	return color;
}
