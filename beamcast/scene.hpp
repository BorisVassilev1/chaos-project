#pragma once

#include <beamcast/data.hpp>
#include <limits>

class Scene {
	public:

	std::vector<Triangle> triangles;

	auto intersect(const Ray& r) const {
		float t = std::numeric_limits<float>::max();
		for (const auto &triangle : triangles) {
			auto tnew = triangle.intersect(r);
			if(tnew > 0.f && tnew < t) {
				t = tnew;
			}
		}
		return t;
	}
};
