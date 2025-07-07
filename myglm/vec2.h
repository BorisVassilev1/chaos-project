#pragma once

#include <myglm/vec.h>

template <class T>
class vec<T, 2> {
   public:
	union {
		T data[2];
		struct {
			union {
				T x, r, s;
			};
			union {
				T y, g, t;
			};
		};
	};

	static constexpr unsigned int N = 2;

#include "vec_basic.inl"

	inline constexpr vec(T &&a1, T &&a2) : data{std::forward<T>(a1), std::forward<T>(a2)} {}

	VEC_SWIZZLE_2(xy, x, y)
	VEC_SWIZZLE_2(xx, x, x)
	VEC_SWIZZLE_2(yy, y, y)
	VEC_SWIZZLE_2(yx, y, x)

};

using vec2	= vec<float, 2>;
using ivec2 = vec<int, 2>;
using uvec2 = vec<unsigned int, 2>;
using dvec2 = vec<double, 2>;
