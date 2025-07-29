#pragma once

#include <myglm/myglm.h>
#include <beamcast/data.hpp>
#include <json/json.hpp>
#include <sample.hpp>

template <class T>
inline constexpr auto toDegrees(T radians) {
	return radians * (180.0f / M_PI);
}

template <class T>
inline constexpr auto toRadians(T degrees) {
	return degrees * (M_PI / 180.0f);
}

inline auto mat4_from_json(const JSONArray &arr) {
	mat4 mat;
	mat[0] = vec4(arr[0].as<JSONNumber>(), arr[1].as<JSONNumber>(), arr[2].as<JSONNumber>(), arr[3].as<JSONNumber>());
	mat[1] = vec4(arr[4].as<JSONNumber>(), arr[5].as<JSONNumber>(), arr[6].as<JSONNumber>(), arr[7].as<JSONNumber>());
	mat[2] = vec4(arr[8].as<JSONNumber>(), arr[9].as<JSONNumber>(), arr[10].as<JSONNumber>(), arr[11].as<JSONNumber>());
	mat[3] = vec4(arr[12].as<JSONNumber>(), arr[13].as<JSONNumber>(), arr[14].as<JSONNumber>(), arr[15].as<JSONNumber>());
	return mat;
}

class Camera {
   public:
	mat4			  view_matrix;
	std::vector<mat4> frames;

	float fov;
	ivec2 resolution;
	float aspect;

	Camera() : view_matrix(identity<float, 4>()), fov(toRadians(90.0f)), resolution{100, 100}, aspect(1.) {}
	Camera(const mat4 &view_matrix, float fov, ivec2 resolution)
		: view_matrix(view_matrix),
		  fov(fov),
		  resolution(resolution),
		  aspect((float)resolution.x / (float)resolution.y) {}

	Camera(const JSONObject &obj) {
		const auto &mat = obj["matrix"].as<JSONArray>();
		view_matrix[0]	= vec4(mat[0].as<JSONNumber>(), mat[1].as<JSONNumber>(), mat[2].as<JSONNumber>(), 0.f);
		view_matrix[1]	= vec4(mat[3].as<JSONNumber>(), mat[4].as<JSONNumber>(), mat[5].as<JSONNumber>(), 0.f);
		view_matrix[2]	= vec4(mat[6].as<JSONNumber>(), mat[7].as<JSONNumber>(), mat[8].as<JSONNumber>(), 0.f);
		view_matrix[3]	= vec4(0, 0, 0, 1);
		view_matrix		= transpose(view_matrix);

		const auto &pos = obj["position"].as<JSONArray>();
		auto		t	= Transpose(view_matrix);
		t[3]			= vec4(pos[0].as<JSONNumber>(), pos[1].as<JSONNumber>(), pos[2].as<JSONNumber>(), 1.f);

		if (obj.find("animation") != obj.end()) {
			auto &transformArrayJSON = obj["animation"].as<JSONArray>();
			for(const auto & mat : transformArrayJSON) {
				frames.push_back(mat4_from_json(mat->as<JSONArray>()));
			}
		}

		if (obj.find("fov") != obj.end()) {
			fov = toRadians(obj["fov"].as<JSONNumber>());
		} else {
			fov = toRadians(90.0f);
		}
	}

	void setResolution(const ivec2 &res) {
		resolution = res;
		// aspect		= (float)resolution.x / (float)resolution.y;
		aspect = (float)resolution.y / (float)resolution.x;
	}

	Ray generate_ray(ivec2 pixel, uint32_t &seed) const {
		pixel.y	 = resolution.y - pixel.y - 1;	   // Flip Y coordinate for image coordinates
		vec2 ndc = vec2(((float)pixel.x + randomFloat(seed)) / (float)resolution.x,
						((float)pixel.y + randomFloat(seed)) / (float)resolution.y);

		vec2 screen = ndc * 2.0f - 1.0f;

		// screen.x *= aspect;
		screen.y *= aspect;

		vec3 direction = normalize(vec3(screen.x * tan(fov / 2.0f), screen.y * tan(fov / 2.0f), -1.0f));
		direction	   = (view_matrix * vec4(direction, 0.0f)).xyz();
		auto t		   = Transpose(view_matrix)[3]._xyz();
		return Ray(t, direction);
	}

	void setFrame(int frame) {
		view_matrix = frames[frame];
	}
};
