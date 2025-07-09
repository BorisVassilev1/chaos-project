#pragma once

#include <emmintrin.h>
#include <myglm/vec.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

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

//inline constexpr auto dot(const vec<float, 3>& v1, const vec<float, 3>& v2) {
//	__m128 a = _mm_set_ps(0.f, v1.z, v1.y, v1.x);
//	__m128 b = _mm_set_ps(0.f, v2.z, v2.y, v2.x);
//	return _mm_cvtss_f32(_mm_dp_ps(a, b, 0x7F));
//}

#define SIMD_OPERATOR_F3(op, f)                                                                      \
	inline constexpr auto operator op(const vec<float, 3>& v1, const vec<float, 3>& v2) {            \
		__m128			  va	 = _mm_set_ps(0.0f, v1.z, v1.y, v1.x);                               \
		__m128			  vb	 = _mm_set_ps(0.0f, v2.z, v2.y, v2.x);                               \
		__m128			  result = f(va, vb);                                                        \
		alignas(16) float temp_result[4];                                                            \
		_mm_maskstore_ps(temp_result, _mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF), result); \
		return vec3{temp_result[0], temp_result[1], temp_result[2]};                                 \
	}
#define SIMD_OPERATOR_INPLACE_F3(op, f)                                                          \
	inline constexpr auto operator op(vec<float, 3>& v1, const vec<float, 3>& v2) {               \
		__m128 va	  = _mm_set_ps(0.0f, v1.z, v1.y, v1.x);                                      \
		__m128 vb	  = _mm_set_ps(0.0f, v2.z, v2.y, v2.x);                                      \
		__m128 result = f(va, vb);                                                      \
		_mm_maskstore_ps(v1.data, _mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF), result); \
	}

//SIMD_OPERATOR_F3(+, _mm_add_ps);
//SIMD_OPERATOR_F3(-, _mm_sub_ps);
//SIMD_OPERATOR_F3(*, _mm_mul_ps);
//SIMD_OPERATOR_F3(/, _mm_div_ps);
//SIMD_OPERATOR_INPLACE_F3(+=, _mm_add_ps);
//SIMD_OPERATOR_INPLACE_F3(-=, _mm_sub_ps);
//SIMD_OPERATOR_INPLACE_F3(*=, _mm_mul_ps);
//SIMD_OPERATOR_INPLACE_F3(/=, _mm_div_ps);

