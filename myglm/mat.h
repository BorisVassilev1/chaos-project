#pragma once

#include <cstddef>
#include <utility>
#include <myglm/vec.h>

template <class T, std::size_t N, std::size_t M>
class mat : public vec<vec<T, M>, N> {
using Base = vec<vec<T, M>, N>;
public:
	using Base::Base;
};

using mat2 = mat<float, 2, 2>;
using mat3 = mat<float, 3, 3>;
using mat4 = mat<float, 4, 4>;
using imat2 = mat<int, 2, 2>;
using imat3 = mat<int, 3, 3>;
using imat4 = mat<int, 4, 4>;
using umat2 = mat<unsigned int, 2, 2>;
using umat3 = mat<unsigned int, 3, 3>;
using umat4 = mat<unsigned int, 4, 4>;
using dmat2 = mat<double, 2, 2>;
using dmat3 = mat<double, 3, 3>;
using dmat4 = mat<double, 4, 4>;

template <class T, std::size_t N, std::size_t M, std::size_t K>
inline constexpr auto operator*(const mat<T, N, M>& m1, const mat<T, M, K>& m2) {
	mat<T, N, K> result;
	for (std::size_t i = 0; i < N; ++i) {
		for (std::size_t j = 0; j < K; ++j) {
			result[i][j] = dot(m1[i], m2[j]);
		}
	}
	return result;
}

template <class T, std::size_t N, std::size_t M>
inline constexpr auto operator*(const mat<T, N, M>& m, const vec<T, M>& v) {
	vec<T, N> result;
	[&]<std::size_t... i>(std::index_sequence<i...>) {
		((result[i] = dot(m[i], v)), ...);
	}(std::make_index_sequence<N>{});
	return result;
}

template <class T, std::size_t N, std::size_t M>
inline constexpr auto operator*(const vec<T, N>& v, const mat<T, N, M>& m) {
	vec<T, M> result;
	auto t = transpose(m);
	[&]<std::size_t... i>(std::index_sequence<i...>) {
		((result[i] = dot(v, t[i])), ...);
	}(std::make_index_sequence<M>{});
	return result;
}

template <class T, std::size_t N, std::size_t M>
inline constexpr auto operator+(const mat<T, N, M>& m1, const mat<T, N, M>& m2) {
	mat<T, N, M> result;
	[&]<std::size_t... i>(std::index_sequence<i...>) {
		((result[i] = m1[i] + m2[i]), ...);
	}(std::make_index_sequence<N>{});
	return result;
}

template <class T, std::size_t N, std::size_t M>
inline constexpr auto operator-(const mat<T, N, M>& m1, const mat<T, N, M>& m2) {
	mat<T, N, M> result;
	[&]<std::size_t... i>(std::index_sequence<i...>) {
		((result[i] = m1[i] - m2[i]), ...);
	}(std::make_index_sequence<N>{});
	return result;
}

template <class T, std::size_t N, std::size_t M>
inline constexpr auto transpose(const mat<T, N, M>& m) {
	mat<T, M, N> result;
	auto it_i = [&] [[clang::always_inline]] (std::size_t i) {
		[&]<std::size_t... j>(std::index_sequence<j...>) {
			((result[j][i] = m[i][j]), ...);
		}(std::make_index_sequence<M>{});
	};

	[&]<std::size_t... i>(std::index_sequence<i...>) {
		((it_i(i)), ...);
	}(std::make_index_sequence<N>{});
	return result;
}

template <class T, std::size_t N>
inline constexpr auto identity() {
	mat<T, N, N> result(0);
	[&]<std::size_t... i>(std::index_sequence<i...>) {
		((result[i][i] = T(1)), ...);
	}(std::make_index_sequence<N>{});
	return result;
}


