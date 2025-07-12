#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <iostream>
#include <type_traits>
#include <cmath>

struct EMPTY {};

template <class T>
concept HasType = requires { typename T::type; };

template <class T, std::size_t N>
class vec {
   public:
	T data[N];

#include "vec_basic.inl"
};

// #define FORCE_INLINE __attribute__((always_inline)) inline
#define FORCE_INLINE inline

template <class T, std::size_t N>
FORCE_INLINE constexpr auto apply_inplace(vec<T, N>& v, auto&& func) {
	[&]<std::size_t... I>(std::index_sequence<I...>) { ((v[I] = func(v[I])), ...); }(std::make_index_sequence<N>{});
	return v;
}

template <class T, std::size_t N>
FORCE_INLINE constexpr auto apply(const vec<T, N>& v, auto&& func) {
	vec<T, N> result = v;
	apply_inplace(result, std::forward<decltype(func)>(func));
	return result;
}

template <class T, std::size_t N>
FORCE_INLINE constexpr auto apply2_inplace(vec<T, N>& v1, const vec<T, N>& v2, auto&& func) {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		((v1[I] = func(v1[I], v2[I])), ...);
	}(std::make_index_sequence<N>{});
	return v1;
}

template <class T, std::size_t N>
FORCE_INLINE constexpr auto apply2(const vec<T, N>& v1, const vec<T, N>& v2, auto&& func) {
	vec<T, N> result = v1;
	apply2_inplace(result, v2, std::forward<decltype(func)>(func));
	return result;
}

template <class T, std::size_t N>
inline constexpr auto operator+(const vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2(v1, v2, std::plus<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator-(const vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2(v1, v2, std::minus<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator*(const vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2(v1, v2, std::multiplies<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator/(const vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2(v1, v2, std::divides<T>{});
}

template <class T>
struct add_scalar {
	const T&				 scalar;
	FORCE_INLINE constexpr T operator()(const T& val) const { return val + scalar; }
};

template <class T>
struct sub_scalar {
	const T&				 scalar;
	FORCE_INLINE constexpr T operator()(const T& val) const { return val - scalar; }
};

template <class T>
struct mul_scalar {
	const T&				 scalar;
	FORCE_INLINE constexpr T operator()(const T& val) const { return val * scalar; }
};

template <class T>
struct div_scalar {
	const T&				 scalar;
	FORCE_INLINE constexpr T operator()(const T& val) const { return val / scalar; }
};

template <class T, std::size_t N>
inline constexpr auto operator+(const vec<T, N>& v, const T& scalar) {
	return apply(v, add_scalar<T>{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator-(const vec<T, N>& v, const T& scalar) {
	return apply(v, sub_scalar<T>{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator*(const vec<T, N>& v, const T& scalar) {
	return apply(v, mul_scalar<T>{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator/(const vec<T, N>& v, const T& scalar) {
	return apply(v, div_scalar<T>{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator+=(vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2_inplace(v1, v2, std::plus<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator-=(vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2_inplace(v1, v2, std::minus<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator*=(vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2_inplace(v1, v2, std::multiplies<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator/(vec<T, N>& v1, const vec<T, N>& v2) {
	return apply2_inplace(v1, v2, std::divides<T>{});
}

template <class T, std::size_t N>
inline constexpr auto operator+=(vec<T, N>& v, const T& scalar) {
	return apply_inplace(v, add_scalar{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator-=(vec<T, N>& v, const T& scalar) {
	return apply_inplace(v, sub_scalar{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator*=(vec<T, N>& v, const T& scalar) {
	return apply_inplace(v, mul_scalar{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator/=(vec<T, N>& v, const T& scalar) {
	return apply_inplace(v, div_scalar{scalar});
}

template <class T, std::size_t N>
inline constexpr auto operator==(const vec<T, N>& u, const vec<T, N>& v) {
	return [&]<std::size_t... I>(std::index_sequence<I...>) {
		return ((u[I] == v[I]) && ...);
	}(std::make_index_sequence<N>{});
}

template <class T, std::size_t N>
inline constexpr auto operator!=(const vec<T, N>& u, const vec<T, N>& v) {
	return !(u == v);
}

template <class T>
struct negate {
	FORCE_INLINE constexpr T operator()(const T& val) const { return -val; }
};

template <class T, std::size_t N>
inline constexpr auto operator-(const vec<T, N>& v) {
	vec<T, N> result = v;
	apply_inplace(result, negate<T>{});
	return result;
}

template <class T, std::size_t N>
std::ostream& operator<<(std::ostream& out, const vec<T, N>& v) {
	out << "(";
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		((out << (I ? "," : "") << v[I]), ...);
	}(std::make_index_sequence<N>{});
	out << ")";
	return out;
}

template <class T, std::size_t N>
inline constexpr auto dot(const vec<T, N>& v1, const vec<T, N>& v2) {
	return [&]<std::size_t... I>(std::index_sequence<I...>) {
		return ((v1[I] * v2[I]) + ...);
	}(std::make_index_sequence<N>{});
}

template <class T, std::size_t N>
inline constexpr auto lengthSquared(const vec<T, N>& v) {
	return dot(v, v);
}

template <class T, std::size_t N>
inline constexpr auto length(const vec<T, N>& v) {
	return std::sqrt(dot(v, v));
}

template <class T, std::size_t N>
inline constexpr auto normalize(const vec<T, N>& v) {
	auto len = length(v);
	if (len == 0) return v;		// Avoid division by zero
	return v / len;
}

template <class T, std::size_t N>
inline constexpr auto mix(const vec<T, N>& v1, const vec<T, N>& v2, T t) {
	return v1 * (T(1.0) - t) + v2 * t;
}

#define VEC_SWIZZLE_2(name, a, b)                                                                                 \
	inline constexpr auto name() { return vec<std::reference_wrapper<value_type>, 2>(std::ref(a), std::ref(b)); } \
	inline constexpr auto name() const { return vec<value_type, 2>(a, b); }                                       \
	inline constexpr auto _##name() const { return vec<value_type, 2>(a, b); }

#define VEC_SWIZZLE_3(name, a, b, c)                                                              \
	inline constexpr auto name()                                                                  \
		requires(!std::is_const_v<value_type>)                                                    \
	{                                                                                             \
		return vec<std::reference_wrapper<value_type>, 3>(std::ref(a), std::ref(b), std::ref(c)); \
	}                                                                                             \
	inline constexpr auto name() const { return vec<value_type, 3>(a, b, c); }                    \
	inline constexpr auto _##name() const { return vec<value_type, 3>(a, b, c); }

#define VEC_SWIZZLE_4(name, a, b, c, d)                                                                        \
	inline constexpr auto name() {                                                                             \
		return vec<std::reference_wrapper<value_type>, 4>(std::ref(a), std::ref(b), std::ref(c), std::ref(d)); \
	}                                                                                                          \
	inline constexpr auto name() const { return vec<value_type, 4>(a, b, c, d); }                              \
	inline constexpr auto _##name() const { return vec<value_type, 4>(a, b, c, d); }
