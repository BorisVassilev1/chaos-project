#include <mesh.hpp>
#include <scene.hpp>
#include <stdexcept>
#include "json/json.hpp"
#include "myglm/mat.h"
#include "util/utils.hpp"

struct FilterFrontFace {
	Ray			ray;
	inline bool operator()(const Triangle& t) const { return dot(ray.direction, t.normal()) < 0.0f; }
};

struct FilterTrue {
	inline bool operator()(const Triangle&) const { return true; }
};

bool Mesh::intersect(const Ray& ray, float tMin, float tMax, RayHit& hit) const {
	assert(bvh.isBuilt() && "BVH must be built before intersection");
	bool res = bvh.intersect(ray, tMin, tMax, hit);
	if (res) { hit.normal = triangleNormals[hit.triangleIndex]; }
	return res;
}

bool Mesh::intersect(const Ray& ray, float tMin, float tMax, RayHit& hit, const BVHType::Filter& filter) const {
	assert(bvh.isBuilt() && "BVH must be built before intersection");
	bool res;
	res = bvh.intersect(ray, tMin, tMax, hit, filter);
	if (res) { hit.normal = triangleNormals[hit.triangleIndex]; }
	return res;
}

MeshObject::MeshObject(const Scene& scene, std::size_t meshIndex, const JSONObject& obj)
	: meshIndex(meshIndex), scene(&scene) {
	transform = identity<float, 4>();
	inverseTransform = identity<float, 4>();
	this->scene = &scene;

	if (obj.find("material_index") != obj.end()) {
		materialIndex = obj["material_index"].as<JSONNumber>() + 1;
	} else {
		materialIndex = 0;	   // Default to first material if not specified TODO: bad
	}

	if(obj.find("transform") != obj.end()) {
		const auto &t = obj["transform"].as<JSONArray>();
		if(t.size() != 16) throw std::runtime_error("wrong number of values in matrix");
		transform = mat4(
			vec4(t[0].as<JSONNumber>(), t[1].as<JSONNumber>(), t[2].as<JSONNumber>(),t[3].as<JSONNumber>()),
			vec4(t[4].as<JSONNumber>(), t[5].as<JSONNumber>(), t[6].as<JSONNumber>(),t[7].as<JSONNumber>()),
			vec4(t[8].as<JSONNumber>(), t[9].as<JSONNumber>(), t[10].as<JSONNumber>(),t[11].as<JSONNumber>()),
			vec4(t[12].as<JSONNumber>(), t[13].as<JSONNumber>(), t[14].as<JSONNumber>(),t[15].as<JSONNumber>()));

		inverseTransform = invert(transform);

		isIdentity = transform == identity<float, 4>();
	}

	const auto& mesh		  = this->scene->meshes[meshIndex];
	vec3		boundsBase[2] = {mesh.box.min, mesh.box.max};
	for (int i = 0; i < 8; ++i) {
		auto boundPoint	 = vec3(boundsBase[(i & 1)].x, boundsBase[(i & 2)].y, boundsBase[(i & 4)].z);
		auto transformed = transform * vec4(boundPoint, 1.0f);
		box.add(transformed.xyz());
	}
	//mesh.expandBox(box);
}

bool MeshObject::intersect(const Ray& ray, float tMin, float tMax, RayHit& intersection) const {
	Ray r(ray);

	if(!isIdentity) {
		r.origin = (inverseTransform * vec4(r.origin, 1.0f)).xyz();
		r.direction = (inverseTransform * vec4(r.direction, 0.0f)).xyz();
	}

	if (materialIndex >= scene->materials.size()) {
		dbLog(dbg::LOG_ERROR, "Material index ", materialIndex, " is out of bounds for scene with ",
			  scene->materials.size(), " materials.");
		assert(false && "Material index out of bounds");
		return false;
	}
	const auto& material = scene->materials[materialIndex];
	if (!material->castsShadows && ray.type == Ray::Shadow) { return false; }

	const auto& mesh = scene->meshes[meshIndex];

	bool res;
	if (material->doubleSided) {
		res = mesh.intersect(r, tMin, tMax, intersection);
	} else {
		res = mesh.intersect(r, tMin, tMax, intersection, FilterFrontFace{ray});
	}
	return res;
}

void MeshObject::fillHitInfo(RayHit& hit, const Ray& ray, bool smooth) const {
	scene->meshes[meshIndex].fillHitInfo(hit, ray, smooth);
	if(!isIdentity)
		hit.normal = (transform * vec4(hit.normal, 0.0f)).xyz();
}
const std::vector<vec3>&  MeshObject::getVertices() const { return scene->meshes[meshIndex].getVertices(); }
const std::vector<vec3>&  MeshObject::getNormals() const { return scene->meshes[meshIndex].getNormals(); }
const std::vector<ivec3>& MeshObject::getIndices() const { return scene->meshes[meshIndex].getIndices(); }
const std::vector<vec3>&  MeshObject::getTriangleNormals() const {
	 return scene->meshes[meshIndex].getTriangleNormals();
}
std::size_t MeshObject::getMaterialIndex() const { return materialIndex; }
