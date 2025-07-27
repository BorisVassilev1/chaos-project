#include <mesh.hpp>
#include <scene.hpp>
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
	if (materialIndex >= scene->materials.size()) {
		dbLog(dbg::LOG_ERROR, "Material index ", materialIndex, " is out of bounds for scene with ",
			  scene->materials.size(), " materials.");
		assert(false && "Material index out of bounds");
		return false;
	}
	const auto& material = scene->materials[materialIndex];
	if (!material->castsShadows && ray.type == Ray::Shadow) { return false; }

	bool res;
	if (material->doubleSided) {
		res = bvh.intersect(ray, tMin, tMax, hit);
	} else {
		res = bvh.intersect(ray, tMin, tMax, hit, FilterFrontFace{ray});
	}
	if (res) { hit.normal = triangleNormals[hit.triangleIndex]; }
	return res;
}
