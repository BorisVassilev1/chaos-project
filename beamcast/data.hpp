#pragma once

#include <myglm/myglm.h>
#include <atomic>
#include <json/json.hpp>
#include <mutex>

struct Ray {
	vec3 origin;
	vec3 direction;

	Ray(const vec3& o, const vec3& d) : origin(o), direction(d) {}

	inline constexpr auto at(float t) const { return origin + direction * t; }
};

struct RayHit {
	vec3		 pos		   = 0;
	float		 t			   = std::numeric_limits<float>::max();
	vec3		 normal		   = 0;
	unsigned int triangleIndex = -1;
	vec2		 uv;
	unsigned int objectIndex = -1;
};

struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;

	Triangle(const vec3& a, const vec3& b, const vec3& c) : v0(a), v1(b), v2(c) {}

	inline constexpr auto normal() const { return cross(v1 - v0, v2 - v0); }

	inline constexpr auto area() const { return length(normal()) * 0.5f; }

	inline constexpr RayHit intersect(const Ray& r) const {
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
		if (u < 0.0 || u > 1.0) [[likely]] return RayHit();
		float v = (uv * wu - uu * wv) * inverseD;
		if (v < 0.0 || (u + v) > 1.0) [[likely]] return RayHit();

		RayHit hit;
		hit.t  = t;
		hit.uv = vec2(u, v);
		return hit;
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

class Mesh {
	std::vector<vec3>  vertices;
	std::vector<vec3>  normals;
	std::vector<vec3>  triangleNormals;
	std::vector<ivec3> indices;

   public:
	Mesh() = default;

	Mesh(const JSONObject& obj) {
		auto& verticesJSON = obj["vertices"].as<JSONArray>();
		for (unsigned int i = 0; i < verticesJSON.size(); i += 3) {
			if (i + 2 >= verticesJSON.size()) {
				throw std::runtime_error("Invalid number of vertices in triangle object");
			}
			vec3 v0 = {verticesJSON[i].as<JSONNumber>(), verticesJSON[i + 1].as<JSONNumber>(),
					   verticesJSON[i + 2].as<JSONNumber>()};
			this->vertices.push_back(v0);
		}
		auto& indicesJSON = obj["triangles"].as<JSONArray>();
		if (indicesJSON.size() % 3 != 0) {
			throw std::runtime_error("Indices must be a multiple of 3 for triangle objects");
		}
		for (unsigned int i = 0; i < indicesJSON.size(); i += 3) {
			if (i + 2 >= indicesJSON.size()) {
				throw std::runtime_error("Invalid number of indices in triangle object");
			}
			unsigned int idx0 = indicesJSON[i].as<JSONNumber>();
			unsigned int idx1 = indicesJSON[i + 1].as<JSONNumber>();
			unsigned int idx2 = indicesJSON[i + 2].as<JSONNumber>();

			if (idx0 >= this->vertices.size() || idx1 >= vertices.size() || idx2 >= vertices.size()) {
				throw std::runtime_error("Index out of bounds in triangle object");
			}

			indices.emplace_back(idx0, idx1, idx2);
			const auto triangle = Triangle(this->vertices[idx0], this->vertices[idx1], this->vertices[idx2]);
			triangleNormals.push_back(normalize(triangle.normal()));
		}
		normals.resize(vertices.size(), vec3(0.0f));
		recalculateNormals();
	}

	void recalculateNormals() {
		std::vector<std::pair<vec3, unsigned int>> normalsSum(vertices.size(), {vec3(0.0f), 0});
		for (const auto& index : indices) {
			const auto triangle = Triangle(vertices[index.x], vertices[index.y], vertices[index.z]);
			vec3	   normal	= normalize(triangle.normal());
			normalsSum[index.x].first += normal;
			normalsSum[index.y].first += normal;
			normalsSum[index.z].first += normal;
			normalsSum[index.x].second++;
			normalsSum[index.y].second++;
			normalsSum[index.z].second++;
		}
		for (const auto& [i, data] : std::views::enumerate(normalsSum)) {
			auto& [normal, count] = data;
			if (count > 0) {
				normal /= float(count);
				normal	   = normalize(normal);
				normals[i] = normal;
			} else {
				std::cerr << "Warning: Normal for vertex with no triangles is zero, setting to default normal."
						  << std::endl;
			}
		}
	}

	inline RayHit intersect(const Ray& ray) const {
		RayHit hit;
		for (auto [i, index] : std::views::enumerate(indices)) {
			const auto triangle = Triangle(vertices[index.x], vertices[index.y], vertices[index.z]);
			const auto hitnew	= triangle.intersect(ray);
			if (hitnew.t > 0.f && hitnew.t < hit.t) {
				hit				  = hitnew;
				hit.triangleIndex = i;
			}
		}
		return hit;
	}

	inline void fillHitInfo(RayHit& hit, const Ray& ray, bool smooth = true) const {
		if (hit.triangleIndex == -1u) return;

		hit.pos = ray.at(hit.t);
		if (smooth) {
			hit.normal = normals[indices[hit.triangleIndex].x] * (1.0f - hit.uv.x - hit.uv.y) +
						 normals[indices[hit.triangleIndex].y] * hit.uv.x +
						 normals[indices[hit.triangleIndex].z] * hit.uv.y;
		} else hit.normal = triangleNormals[hit.triangleIndex];
	}
};

class PercentLogger {
	std::string			name;
	std::size_t			total;
	std::atomic_int64_t current;

	std::mutex mutex;

   public:
	PercentLogger(const std::string& name, std::size_t total) : name(name), total(total), current(0) {
		std::lock_guard lock(mutex);
		std::cout << name << ": 0%" << std::flush;
	}

	inline constexpr void step() {
		current.fetch_add(1, std::memory_order_relaxed);
		if (current.load(std::memory_order_relaxed) % (total / 100) == 0) {
			std::lock_guard lock(mutex);
			std::cout << "\r" << name << ": " << (current * 100 / total) << "%" << std::flush;
		}
	}

	inline constexpr void finish() {
		std::lock_guard lock(mutex);
		std::cout << "\r" << name << ": 100%" << std::endl;
	}
};
