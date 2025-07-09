#pragma once

#include <myglm/vec.h>
#include <immintrin.h>

template <class T>
class vec<T, 4> {
   public:
	union {
		T data[4];
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
			union {
				T w, a, q;
			};
		};
	};

	static constexpr unsigned int N = 4;

#include "vec_basic.inl"

	inline constexpr vec(T&& a1, T&& a2, T&& a3, T&& a4)
		: data{std::forward<T>(a1), std::forward<T>(a2), std::forward<T>(a3), std::forward<T>(a4)} {}

	VEC_SWIZZLE_2(xy, x, y)
	VEC_SWIZZLE_2(xz, x, z)
	VEC_SWIZZLE_2(xw, x, w)
	VEC_SWIZZLE_2(yx, y, x)
	VEC_SWIZZLE_2(yz, y, z)
	VEC_SWIZZLE_2(yw, y, w)
	VEC_SWIZZLE_2(zx, z, x)
	VEC_SWIZZLE_2(zy, z, y)
	VEC_SWIZZLE_2(zw, z, w)
	VEC_SWIZZLE_2(wx, w, x)
	VEC_SWIZZLE_2(wy, w, y)
	VEC_SWIZZLE_2(wz, w, z)

	VEC_SWIZZLE_2(xx, x, x)
	VEC_SWIZZLE_2(yy, y, y)
	VEC_SWIZZLE_2(zz, z, z)
	VEC_SWIZZLE_2(ww, w, w)

	VEC_SWIZZLE_3(xyz, x, y, z)
	VEC_SWIZZLE_3(xyw, x, y, w)
	VEC_SWIZZLE_3(xzw, x, z, w)
	VEC_SWIZZLE_3(yxz, y, x, z)
	VEC_SWIZZLE_3(yxw, y, x, w)
	VEC_SWIZZLE_3(yzw, y, z, w)
	VEC_SWIZZLE_3(zxy, z, x, y)
	VEC_SWIZZLE_3(zxw, z, x, w)
	VEC_SWIZZLE_3(zyx, z, y, x)
	VEC_SWIZZLE_3(zyw, z, y, w)
	VEC_SWIZZLE_3(wxz, w, x, z)
	VEC_SWIZZLE_3(wxy, w, x, y)
	VEC_SWIZZLE_3(wyz, w, y, z)

	VEC_SWIZZLE_4(xyzw, x, y, z, w)
	VEC_SWIZZLE_4(xywz, x, y, w, z)
	VEC_SWIZZLE_4(xzyw, x, z, y, w)
	VEC_SWIZZLE_4(xzwy, x, z, w, y)
	VEC_SWIZZLE_4(xwyz, x, w, y, z)
	VEC_SWIZZLE_4(xwzy, x, w, z, y)

	VEC_SWIZZLE_4(yxzw, y, x, z, w)
	VEC_SWIZZLE_4(yxwz, y, x, w, z)
	VEC_SWIZZLE_4(yzxw, y, z, x, w)
	VEC_SWIZZLE_4(yzwx, y, z, w, x)
	VEC_SWIZZLE_4(ywxz, y, w, x, z)
	VEC_SWIZZLE_4(ywzx, y, w, z, x)

	VEC_SWIZZLE_4(zxyw, z, x, y, w)
	VEC_SWIZZLE_4(zxwy, z, x, w, y)
	VEC_SWIZZLE_4(zyxw, z, y, x, w)
	VEC_SWIZZLE_4(zywx, z, y, w, x)
	VEC_SWIZZLE_4(zwxy, z, w, x, y)
	VEC_SWIZZLE_4(zwyx, z, w, y, x)

	VEC_SWIZZLE_4(wxzy, w, x, z, y)
	VEC_SWIZZLE_4(wxyz, w, x, y, z)
	VEC_SWIZZLE_4(wyxz, w, y, x, z)
	VEC_SWIZZLE_4(wyzx, w, y, z, x)
	VEC_SWIZZLE_4(wzxy, w, z, x, y)
	VEC_SWIZZLE_4(wzyx, w, z, y, x)
};

using vec4	= vec<float, 4>;
using ivec4 = vec<int, 4>;
using uvec4 = vec<unsigned int, 4>;
using dvec4 = vec<double, 4>;

#define SIMD_OPERATOR_F4(op, f)                                                           \
	inline constexpr auto operator op(const vec<float, 4>& v1, const vec<float, 4>& v2) { \
		__m128 va	  = _mm_load_ps(v1.data);                                             \
		__m128 vb	  = _mm_load_ps(v2.data);                                             \
		__m128 result = f(va, vb);                                                        \
		vec4   v_result;                                                                  \
		_mm_store_ps(v_result.data, result);                                              \
	}
#define SIMD_OPERATOR_INPLACE_F4(op, f)                                              \
	inline constexpr auto& operator op(vec<float, 3>& v1, const vec<float, 3>& v2) { \
		__m128 va	  = _mm_load_ps(v1.data);                                        \
		__m128 vb	  = _mm_load_ps(v2.data);                                        \
		__m128 result = f(va, vb);                                                   \
		_mm_store_ps(v1.data, result);                                               \
		return v1;                                                                   \
	}

#define SIMD_OPERATOR_F4_SCALAR(op, f)                                     \
	inline constexpr auto operator op(const vec<float, 4>& v1, float v2) { \
		__m128 va	  = _mm_load_ps(v1.data);                              \
		__m128 vb	  = _mm_set1_ps(v2);                                   \
		__m128 result = f(va, vb);                                         \
		vec4   v_result;                                                   \
		_mm_store_ps(v_result.data, result);                               \
		return v_result;                                                   \
	}

#define SIMD_OPERATOR_INPLACE_F4_SCALAR(op, f)                        \
	inline constexpr auto& operator op(vec<float, 4>& v1, float v2) { \
		__m128 va	  = _mm_load_ps(v1.data);                         \
		__m128 vb	  = _mm_set1_ps(v2);                              \
		__m128 result = f(va, vb);                                    \
		_mm_store_ps(v1.data, result);                                \
		return v1;                                                    \
	}

SIMD_OPERATOR_F4(+, _mm_add_ps);
SIMD_OPERATOR_F4(-, _mm_sub_ps);
SIMD_OPERATOR_F4(*, _mm_mul_ps);
SIMD_OPERATOR_F4(/, _mm_div_ps);
SIMD_OPERATOR_INPLACE_F4(+=, _mm_add_ps);
SIMD_OPERATOR_INPLACE_F4(-=, _mm_sub_ps);
SIMD_OPERATOR_INPLACE_F4(*=, _mm_mul_ps);
SIMD_OPERATOR_INPLACE_F4(/=, _mm_div_ps);

inline constexpr auto dot(const vec<float, 4>& v1, const vec<float, 4>& v2) {
	__m128			  a		 = _mm_load_ps(v1.data);
	__m128			  b		 = _mm_load_ps(v2.data);
	__m128			  result = _mm_dp_ps(a, b, 0xF);
	alignas(16) float temp_result;
	_mm_store_ps1(&temp_result, result);
	return temp_result;
}

SIMD_OPERATOR_F4_SCALAR(+, _mm_add_ps);
SIMD_OPERATOR_F4_SCALAR(-, _mm_sub_ps);
SIMD_OPERATOR_F4_SCALAR(*, _mm_mul_ps);
SIMD_OPERATOR_F4_SCALAR(/, _mm_div_ps);
SIMD_OPERATOR_INPLACE_F4_SCALAR(+=, _mm_add_ps);
SIMD_OPERATOR_INPLACE_F4_SCALAR(-=, _mm_sub_ps);
SIMD_OPERATOR_INPLACE_F4_SCALAR(*=, _mm_mul_ps);
SIMD_OPERATOR_INPLACE_F4_SCALAR(/=, _mm_div_ps);

