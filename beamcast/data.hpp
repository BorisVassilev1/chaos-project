#pragma once

#include <myglm/myglm.h>
#include <json/json.hpp>
#include <util/utils.hpp>
#include <float.h>

struct Ray {
	vec3 origin;
	vec3 direction;
	vec3 attenuation;
	enum Type {
		Primary,
		Shadow,
	} type = Type::Primary;

	Ray(const vec3& o, const vec3& d, Type t = Primary, const vec3 &attenuation = 1.0f) : origin(o), direction(d), attenuation(attenuation), type(t) {}

	inline constexpr auto at(float t) const { return origin + direction * t; }
};

struct RayHit {
	vec3		 pos		   = 0;
	float		 t			   = std::numeric_limits<float>::max();
	vec3		 normal		   = 0;
	unsigned int triangleIndex = -1;
	vec2		 uv;
	unsigned int objectIndex = -1;
	unsigned int depth		 = 0;
	vec3		 texCoords	 = 0;
};

/**
 * struct BBox - Axis Aligned Bouding Box
 */
struct AABB {
	vec3 min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	vec3 max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	AABB() = default;

	AABB(const vec3& min, const vec3& max) : min(min), max(max) {
		assert(min.x <= max.x && min.y <= max.y && min.z <= max.z);
	}

	/// @brief Checks if the box is small enough to be ignored
	/// @return true if it is small enough, false otherwise
	bool isEmpty() const {
		const vec3 size = max - min;
		return size.x <= 1e-6f || size.y <= 1e-6f || size.z <= 1e-6f;
	}

	/// @brief Extends this AABB to include both itself and \a other
	void add(const AABB& other) {
		min = ::min(min, other.min);
		max = ::max(max, other.max);
	}

	/// @brief Expand the box with a single point
	void add(const vec3& point) {
		min = ::min(min, point);
		max = ::max(max, point);
	}

	/// @brief Checks if a point is in this Bounding Box
	bool inside(const vec3& point) const {
		return (min.x - 1e-6 <= point.x && point.x <= max.x + 1e-6 && min.y - 1e-6 <= point.y &&
				point.y <= max.y + 1e-6 && min.z - 1e-6 <= point.z && point.z <= max.z + 1e-6);
	}

	/// @brief Split the box in 8 equal parts, children are not sorted in any way
	/// @param parts [out] - where to write the children
	// void octSplit(AABB parts[8]) const;

	/// @brief Compute the intersection with another box
	///	@return empty box if there is no intersection
	AABB boxIntersection(const AABB& other) const {
		assert(!isEmpty());
		assert(!other.isEmpty());
		return {::max(min, other.min), ::min(max, other.max)};
	}

	/// @brief Check if a ray intersects the box and find the distance
	bool testIntersect(const Ray& ray, float& t) const {
		// Source:
		// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
		float t1 = -FLT_MAX;
		float t2 = FLT_MAX;

		vec3 one_over_raydir = vec3(1.0f) / ray.direction;
		vec3 t0s			 = mult_safe((min - ray.origin), one_over_raydir);
		vec3 t1s			 = mult_safe((max - ray.origin), one_over_raydir);

		vec3 tsmaller = ::min(t0s, t1s);
		vec3 tbigger  = ::max(t0s, t1s);

		t1 = std::max(t1, std::max(tsmaller.x, std::max(tsmaller.y, tsmaller.z)));
		t2 = std::min(t2, std::min(tbigger.x, std::min(tbigger.y, tbigger.z)));
		t  = t1;
		return t1 <= t2;
	}

	/// @brief Check if a ray intersects the box
	bool testIntersect(const Ray& ray) const {
		float a;
		return testIntersect(ray, a);
	}

	/// @brief get the center of the box
	vec3 center() const { return (min + max) * 0.5f; }

	/// @brief calculate the surface area of the AABB. Used for SAH
	float surfaceArea() const {
		const vec3 size = max - min;
		return (size.x * size.y + size.x * size.z + size.y * size.z);
	}
};

struct PointLight {
	vec3  position;
	float intensity = 1.0f;
	vec3  color;

	PointLight(const vec3& pos, const vec3& col, float intensity = 1.0f)
		: position(pos), intensity(intensity), color(col) {}

	PointLight(const JSONObject& obj) {
		const auto& pos = obj["position"].as<JSONArray>();
		position		= vec3{pos[0].as<JSONNumber>(), pos[1].as<JSONNumber>(), pos[2].as<JSONNumber>()};

		intensity = obj["intensity"].as<JSONNumber>();

		color = vec3{1.0f, 1.0f, 1.0f};
	}
};
