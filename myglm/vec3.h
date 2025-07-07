#pragma once

#include <myglm/vec.h>

template <class T>
class vec<T, 3> {
   public:
	union {
		T data[3];
		struct {
			union {
				T x, r, s;
			};
			union {
				T y, g, t;
			};
			union {
				T z, b, p;
			};
		};
	};

	static constexpr unsigned int N = 3;

#include "vec_basic.inl"

	inline constexpr vec(T&& a1, T&& a2, T&& a3)
		: data{std::forward<T>(a1), std::forward<T>(a2), std::forward<T>(a3)} {}

	VEC_SWIZZLE_2(xy, x, y)
	VEC_SWIZZLE_2(xz, x, z)
	VEC_SWIZZLE_2(yx, y, x)
	VEC_SWIZZLE_2(yz, y, z)
	VEC_SWIZZLE_2(zx, z, x)
	VEC_SWIZZLE_2(zy, z, y)

	VEC_SWIZZLE_3(xyz, x, y, z)
	VEC_SWIZZLE_3(xzy, x, z, y)
	VEC_SWIZZLE_3(yxz, y, x, z)
	VEC_SWIZZLE_3(yzx, y, z, x)
	VEC_SWIZZLE_3(zxy, z, x, y)
	VEC_SWIZZLE_3(zyx, z, y, x)

	VEC_SWIZZLE_3(xxx, x, x, x)
	VEC_SWIZZLE_3(yyy, y, y, y)
	VEC_SWIZZLE_3(zzz, z, z, z)
};

using vec3	= vec<float, 3>;
using ivec3 = vec<int, 3>;
using uvec3 = vec<unsigned int, 3>;
using dvec3 = vec<double, 3>;

template <class T>
inline constexpr auto cross(const vec<T, 3>& v1, const vec<T, 3>& v2) {
	return vec<T, 3>{v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

template <class T>
inline constexpr auto triple(const vec<T, 3>& u, const vec<T, 3>& v, const vec<T, 3>& w) {
	return dot(u, cross(v, w));
}
