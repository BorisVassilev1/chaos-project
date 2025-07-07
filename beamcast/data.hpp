#pragma once

#include <myglm/myglm.h>

struct Ray {
	vec3 origin;
	vec3 direction;

	Ray(const vec3& o, const vec3& d) : origin(o), direction(d) {}

	inline constexpr auto at(float t) const { return origin + direction * t; }
};

struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;

	Triangle(const vec3& a, const vec3& b, const vec3& c) : v0(a), v1(b), v2(c) {}

	inline constexpr auto normal() const { return cross(v1 - v0, v2 - v0); }

	inline constexpr auto area() const { return length(normal()) * 0.5f; }

	inline constexpr auto intersect(const Ray& r) const {
		// graphicon.org/html/2012/conference/EN2%20-%20Graphics/gc2012Shumskiy.pdf
		vec3  e1	 = v1 - v0;
		vec3  e2	 = v2 - v0;
		vec3  normal = normalize(cross(e1, e2));
		float b		 = dot(normal, r.direction);
		vec3  w0	 = r.origin - v0;
		float a		 = -dot(normal, w0);
		float t		 = a / b;
		vec3  p		 = r.origin + r.direction * t;
		float uu, uv, vv, wu, wv, inverseD;
		uu		 = dot(e1, e1);
		uv		 = dot(e1, e2);
		vv		 = dot(e2, e2);
		vec3 w	 = p - v0;
		wu		 = dot(w, e1);
		wv		 = dot(w, e2);
		inverseD = uv * uv - uu * vv;
		inverseD = 1.0f / inverseD;
		float u	 = (uv * wv - vv * wu) * inverseD;
		if (u < 0.0 || u > 1.0) return -1.0f;
		float v = (uv * wu - uu * wv) * inverseD;
		if (v < 0.0 || (u + v) > 1.0) return -1.0f;

		// UV = vec2(u, v);
		return t;
	}
};
