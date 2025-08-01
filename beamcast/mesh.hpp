#pragma once
#include <intersectable.hpp>
#include <bvh.hpp>

class Scene;

class Mesh : public Primitive {
	std::vector<vec3>  vertices;
	std::vector<vec3>  normals;
	std::vector<vec3>  texCoords;
	std::vector<vec3>  triangleNormals;
	std::vector<ivec3> indices;

	using BVHType = ygl::bvh::TriangleBVH;

	BVHType bvh;

   public:
	Mesh()						 = delete;
	Mesh(const Mesh&)			 = delete;
	Mesh(Mesh&&)				 = default;
	Mesh& operator=(const Mesh&) = delete;
	Mesh& operator=(Mesh&&)		 = default;

	Mesh(const JSONObject& obj) {
		auto& verticesJSON = obj["vertices"].as<JSONArray>();
		this->vertices.reserve(verticesJSON.size() / 3);
		for (unsigned int i = 0; i < verticesJSON.size(); i += 3) {
			if (i + 2 >= verticesJSON.size()) {
				throw std::runtime_error("Invalid number of vertices in triangle object");
			}
			vec3 v0 = {verticesJSON[i].as<JSONNumber>(), verticesJSON[i + 1].as<JSONNumber>(),
					   verticesJSON[i + 2].as<JSONNumber>()};
			this->vertices.push_back(v0);
			this->box.add(v0);
		}

		this->texCoords.reserve(vertices.size());
		if (obj.find("uvs") == obj.end()) {
			texCoords.resize(vertices.size(), vec3(0.0f));
			dbLog(dbg::LOG_WARNING, "No texture coordinates found in triangle object.");
		} else {
			auto& texCoordsJSON = obj["uvs"].as<JSONArray>();
			assert(texCoordsJSON.size() == verticesJSON.size());
			for (unsigned int i = 0; i < texCoordsJSON.size(); i += 3) {
				if (i + 2 >= texCoordsJSON.size()) {
					throw std::runtime_error("Invalid number of texture coordinates in triangle object");
				}
				vec3 uv = {texCoordsJSON[i].as<JSONNumber>(), texCoordsJSON[i + 1].as<JSONNumber>(),
						   texCoordsJSON[i + 2].as<JSONNumber>()};
				this->texCoords.push_back(uv);
			}
		}

		auto& indicesJSON = obj["triangles"].as<JSONArray>();
		if (indicesJSON.size() % 3 != 0) {
			throw std::runtime_error("Indices must be a multiple of 3 for triangle objects");
		}

		this->indices.reserve(indicesJSON.size() / 3);
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
			const auto triangle = Triangle(this->vertices[idx0], this->vertices[idx1], this->vertices[idx2], i / 3);
			triangleNormals.push_back(normalize(triangle.normal()));
			bvh.addPrimitive(triangle);
		}
		normals.resize(vertices.size(), vec3(0.0f));
		recalculateNormals();
		bvh.build(BVHType::Purpose::Mesh);

		dbLog(dbg::LOG_DEBUG, "Mesh created with ", vertices.size(), " vertices and ", indices.size(), " triangles.");
	}

	void recalculateNormals() {
		std::vector<std::pair<vec3, unsigned int>> normalsSum(vertices.size(), {vec3(0.0f), 0});
		for (const auto& index : indices) {
			const auto triangle = Triangle(vertices[index.x], vertices[index.y], vertices[index.z], 0);
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
				dbLog(dbg::LOG_WARNING, "Normal for vertex ", i, " has no triangles, setting to default normal.");
			}
		}
	}

	bool intersect(const Ray& ray, float tMin, float tMax, RayHit& hit) const override;
	bool intersect(const Ray& ray, float tMin, float tMax, RayHit& hit, const BVHType::Filter& filter) const;

	inline void fillHitInfo(RayHit& hit, const Ray& ray, bool smooth = true) const {
		if (hit.triangleIndex == -1u) return;

		hit.pos = ray.at(hit.t);
		if (smooth) {
			hit.normal = normalize(normals[indices[hit.triangleIndex].x] * (1.0f - hit.uv.x - hit.uv.y) +
								   normals[indices[hit.triangleIndex].y] * hit.uv.x +
								   normals[indices[hit.triangleIndex].z] * hit.uv.y);
		} else hit.normal = triangleNormals[hit.triangleIndex];

		hit.texCoords = texCoords[indices[hit.triangleIndex].x] * (1.0f - hit.uv.x - hit.uv.y) +
						texCoords[indices[hit.triangleIndex].y] * hit.uv.x +
						texCoords[indices[hit.triangleIndex].z] * hit.uv.y;
		if (isnan(hit.normal)) {
			dbLog(dbg::LOG_ERROR, "triangle normal invalid: ", hit.normal);
			dbLog(dbg::LOG_ERROR, "ray: ", ray.origin, ray.direction);
			dbLog(dbg::LOG_ERROR, "hit: ", hit.pos, hit.t, hit.normal, hit.uv, hit.triangleIndex, hit.objectIndex);
			dbLog(dbg::LOG_ERROR, "normals: ", normals[indices[hit.triangleIndex].x],
				  normals[indices[hit.triangleIndex].y], normals[indices[hit.triangleIndex].z]);
		}
	}

	inline constexpr auto& getVertices() const { return vertices; }
	inline constexpr auto& getNormals() const { return normals; }
	inline constexpr auto& getIndices() const { return indices; }
	inline constexpr auto& getTriangleNormals() const { return triangleNormals; }

	void writeTo(char*, std::size_t) override {
		assert(false && "Mesh::writeTo not implemented yet"); /* TODO: implement */
	}

	void print(std::ostream&) const override {
		assert(false && "Mesh::print not implemented yet"); /* TODO: implement */
	}
};

class MeshObject : public Primitive {
   public:
	std::size_t	 meshIndex = 0;
	mat4		 transform;
	mat4		inverseTransform;
	const Scene* scene;
	std::size_t materialIndex = 0;
	bool isIdentity = true;

	MeshObject(const Scene& scene, std::size_t meshIndex, const JSONObject& obj);

	bool intersect(const Ray& ray, float tMin, float tMax, RayHit& intersection) const override;

	inline void writeTo(char*, std::size_t) override { assert(false); }

	inline void print(std::ostream&) const override { assert(false && "Object::print not implemented yet"); }

	void								fillHitInfo(RayHit& hit, const Ray& ray, bool smooth = true) const;
	const std::vector<vec3>&	getVertices() const;
	const std::vector<vec3>&	getNormals() const;
	const std::vector<ivec3>& getIndices() const;
	const std::vector<vec3>&	getTriangleNormals() const;
	std::size_t				getMaterialIndex() const;
};
