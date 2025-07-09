#pragma once

#include <myglm/myglm.h>
#include <beamcast/data.hpp>
#include <json/json.hpp>

template <class T>
inline constexpr auto toDegrees(T radians) {
	return radians * (180.0f / M_PI);
}

template <class T>
inline constexpr auto toRadians(T degrees) {
	return degrees * (M_PI / 180.0f);
}

class Camera {
   public:
	mat4 view_matrix;

	float fov;

	Camera() : view_matrix(identity<float, 4>()), fov(toRadians(60.0f)) {}
	Camera(const mat4 &view_matrix, float fov) : view_matrix(view_matrix), fov(fov) {}

	Camera(const JSONObject &obj) {
		const auto &mat = obj["matrix"].as<JSONArray>();
		view_matrix[0]	= vec4(mat[0].as<JSONNumber>(), mat[1].as<JSONNumber>(), mat[2].as<JSONNumber>(), 0.f);
		view_matrix[1]	= vec4(mat[3].as<JSONNumber>(), mat[4].as<JSONNumber>(), mat[5].as<JSONNumber>(), 0.f);
		view_matrix[2]	= vec4(mat[6].as<JSONNumber>(), mat[7].as<JSONNumber>(), mat[8].as<JSONNumber>(), 0.f);
		view_matrix[3]	= vec4(0, 0, 0, 1);

		const auto &pos = obj["position"].as<JSONArray>();
		auto		t	= Transpose(view_matrix);
		t[3]			= vec4(pos[0].as<JSONNumber>(), pos[1].as<JSONNumber>(), pos[2].as<JSONNumber>(), 1.f);

		fov = toRadians(65.0f);
	}

	Ray generate_ray(ivec2 pixel, ivec2 resolution) const {
		pixel.y = resolution.y - pixel.y - 1; // Flip Y coordinate for image coordinates
		float aspect = (float)resolution.x / (float)resolution.y;
		vec2  ndc	 = vec2(((float)pixel.x + .5f) / (float)resolution.x, ((float)pixel.y + .5f) / (float)resolution.y);

		vec2 screen = vec2(ndc.x * 2.0f - 1.0f, ndc.y * 2.0f - 1.0f);

		screen.x *= aspect;

		vec3 direction = normalize(vec3(screen.x * tan(fov / 2.0f), screen.y * tan(fov / 2.0f), -1.0f));
		auto t		   = Transpose(view_matrix)[3]._xyz();
		return Ray(t, direction);
	}
};
