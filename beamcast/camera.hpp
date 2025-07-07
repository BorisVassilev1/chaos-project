#pragma once 

#include <myglm/myglm.h>
#include <beamcast/data.hpp>

class Camera {
public:

	mat4 view_matrix;

	float fov;
	ivec2 resolution;

	Ray generate_ray(ivec2 pixel) const {
		float aspect = (float)resolution.x / (float)resolution.y;
		vec2 ndc = vec2(
			((float)pixel.x + .5f) / (float)resolution.x,
			((float)pixel.y + .5f) / (float)resolution.y
		);
		
		vec2 screen = vec2(
			ndc.x * 2.0f - 1.0f,
			ndc.y * 2.0f - 1.0f
		);

		screen.x *= aspect;
		
		vec3 direction = normalize(vec3(
			screen.x * tan(fov / 2.0f),
			screen.y * tan(fov / 2.0f),
			-1.0f
		));
		auto t = Transpose(view_matrix)[3]._xyz();


		return Ray(vec3(0.f), direction);
	}

};
