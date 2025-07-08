#pragma once

#include <myglm/mat.h>

template <class T, std::size_t N, std::size_t M, class type>
class Transpose {
   public:
	// using type = mat<T, M, N>;
	using value_type = std::conditional_t<std::is_const_v<type>, const T, T>;

	template <class U>
	Transpose(U&& matrix) : m(matrix) {}

	constexpr inline auto operator[](std::size_t i)
		requires(!std::is_const_v<type>)
	{
		return [&]<std::size_t... j>(std::index_sequence<j...>) {
			return vec<std::reference_wrapper<value_type>, N>(std::ref(m[j][i])...);
		}(std::make_index_sequence<N>{});
	}

	constexpr inline auto operator[](std::size_t i) const {
		return [&]<std::size_t... j>(std::index_sequence<j...>) {
			return vec<std::reference_wrapper<const T>, N>(std::ref(m[j][i])...);
		}(std::make_index_sequence<N>{});
	}

	constexpr inline std::size_t size() const { return N; }

   private:
	type& m;
};

template <class T, std::size_t N, std::size_t M>
Transpose(const mat<T, N, M>& matrix) -> Transpose<T, N, M, const mat<T, N, M>>;

template <class T, std::size_t N, std::size_t M>
Transpose(mat<T, N, M>& matrix) -> Transpose<T, N, M, mat<T, N, M>>;
