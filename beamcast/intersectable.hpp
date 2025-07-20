#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <data.hpp>

struct Intersectable {
	/// @brief Called after scene is fully created and before rendering starts
	///	       Used to build acceleration structures
	virtual void onBeforeRender() {}

	/// @brief Default implementation intersecting the bbox of the primitive, overriden if possible more efficiently
	virtual bool boxIntersect(const AABB &other) = 0;

	/// @brief Default implementation adding the whole bbox, overriden for special cases
	virtual void expandBox(AABB &other) const = 0;

	/// @brief Get the center
	virtual vec3 getCenter() const = 0;

	/**
	 * @brief Writes the Intersectable to \a buff at position \a offset
	 *
	 * @param buff - buffer to write to
	 * @param offset - offset from the beginning of \a buff to write at
	 */
	virtual void writeTo(char *buff, std::size_t offset) = 0;
	/**
	 * @brief Prints the Intersectable in human-readable text format to \a os
	 *
	 * @param os - stream to write to
	 */
	virtual void print(std::ostream &os) const = 0;

	/// @brief Intersect a ray with the primitive allowing intersection in (tMin, tMax) along the ray
	/// @param ray - the ray
	/// @param tMin - near clip distance
	/// @param tMax - far clip distance
	/// @param intersection [out] - data for intersection if one is found
	/// @return true when intersection is found, false otherwise
	virtual bool intersect(const Ray &ray, float tMin, float tMax, RayHit &intersection) const = 0;

	virtual ~Intersectable() = default;
};

/// Base class for scene object
struct Primitive : Intersectable {
	AABB box;

	/// @brief Default implementation intersecting the bbox of the primitive, overriden if possible more efficiently
	bool boxIntersect(const AABB &other) { return !box.boxIntersection(other).isEmpty(); }

	/// @brief Default implementation adding the whole bbox, overriden for special cases
	void expandBox(AABB &other) const { other.add(box); }

	/// @brief Get the center
	vec3 getCenter() const { return box.center(); }

	~Primitive() = default;
};

struct Triangle : public Primitive {
	vec3		v0;
	vec3		v1;
	vec3		v2;
	uint_fast32_t index;

	operator bool() const { return index != -1u; }

	Triangle(std::nullptr_t) : index(-1u) {
		assert(!*this);
	}
	Triangle(const vec3 &a, const vec3 &b, const vec3 &c, uint_fast32_t index = -1u) : v0(a), v1(b), v2(c), index(index) {
		// box.add(v0);
		// box.add(v1);
		// box.add(v2);
	}
	void expandBox(AABB &other) const {
		other.add(v0);
		other.add(v1);
		other.add(v2);
	}
	vec3 getCenter() const { return (v0 + v1 + v2) / 3.0f; }

	inline constexpr auto normal() const { return cross(v1 - v0, v2 - v0); }

	inline constexpr auto area() const { return length(normal()) * 0.5f; }

	virtual bool intersect(const Ray &r, float tMin, float tMax, RayHit &hit) const {
		// graphicon.org/html/2012/conference/EN2%20-%20Graphics/gc2012Shumskiy.pdf
		vec3  e1	 = v1 - v0;
		vec3  e2	 = v2 - v0;
		vec3  normal = normalize(cross(e1, e2));
		float b		 = dot(normal, r.direction);
		vec3  w0	 = r.origin - v0;
		float a		 = -dot(normal, w0);
		float t		 = a / b;
		if (t < tMin || t > tMax) return false;
		vec3  p = r.origin + r.direction * t;
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
		if (u < 0.0 || u > 1.0) [[likely]]
			return false;
		float v = (uv * wu - uu * wv) * inverseD;
		if (v < 0.0 || (u + v) > 1.0) [[likely]]
			return false;

		hit.t			  = t;
		hit.uv			  = vec2(u, v);
		hit.triangleIndex = index;
		return true;
	}

	inline void writeTo(char *buff, std::size_t offset) {
		std::memcpy(buff + offset, &v0, sizeof(vec3));
		std::memcpy(buff + offset + sizeof(vec3), &v1, sizeof(vec3));
		std::memcpy(buff + offset + 2 * sizeof(vec3), &v2, sizeof(vec3));
	}

	inline void print(std::ostream &os) const {
		os << "Triangle: v0 = " << v0 << ", v1 = " << v1 << ", v2 = " << v2 << ", index = " << index << std::endl;
		;
	}
};
