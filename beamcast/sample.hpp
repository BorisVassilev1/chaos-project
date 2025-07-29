#pragma once

#include <cstdint>
#include <myglm/myglm.h>

inline float RadicalInverse_VdC(uint32_t bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;	 // / 0x100000000
}

inline auto hammersley(uint32_t i, uint32_t N) -> vec2 { return vec2(float(i) / float(N), RadicalInverse_VdC(i)); }

inline uint pcg_hash(uint input) {
	uint state = input * 747796405u + 2891336453u;
	uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

inline float randomFloat(uint32_t &seed) {
	seed = (seed * 1103515245 + 12345) & 0x7fffffff;
	return float(seed) / float(0x7fffffff);
}

