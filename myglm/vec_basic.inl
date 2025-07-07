
static auto type()
	requires(!HasType<T>)
{
	return T{};
}
static auto type()
	requires(HasType<T>)
{
	return (typename T::type){};
}

using value_type = decltype(type());

inline constexpr vec() = default;
template <typename... Args>
	requires(sizeof...(Args) == N)
inline constexpr vec(Args&&... args) {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		((data[I] = std::forward<Args>(args)), ...);
	}(std::make_index_sequence<N>{});
}
inline constexpr vec(T&& arg) {
	[&]<std::size_t... I>(std::index_sequence<I...>) { ((data[I] = arg), ...); }(std::make_index_sequence<N>{});
}

template <typename... Args>
	requires(sizeof...(Args) == N - 2)
inline constexpr vec(vec<T, 2>&& v, Args&&... args)
	: data{std::move(v.data[0]), std::move(v.data[1]), std::forward<Args>(args)...} {}

template <typename... Args>
	requires(sizeof...(Args) == N - 3)
inline constexpr vec(vec<T, 3>&& v, Args&&... args)
	: data{std::move(v.data[0]), std::move(v.data[1]), std::move(v.data[2]), std::forward<Args>(args)...} {}

template <typename... Args>
	requires(sizeof...(Args) == N - 4)
inline constexpr vec(vec<T, 2>&& v1, vec<T, 2>&& v2, Args&&... args)
	: data{std::move(v1.data[0]), std::move(v1.data[1]), std::move(v2.data[0]), std::move(v2.data[1]),
		   std::forward<Args>(args)...} {}

template <typename... Args>
	requires(sizeof...(Args) == N - 2)
inline constexpr vec(const vec<T, 2>& v, Args&&... args)
	: data{std::move(v.data[0]), std::move(v.data[1]), std::forward<Args>(args)...} {}

template <typename... Args>
	requires(sizeof...(Args) == N - 3)
inline constexpr vec(const vec<T, 3>& v, Args&&... args)
	: data{std::move(v.data[0]), std::move(v.data[1]), std::move(v.data[2]), std::forward<Args>(args)...} {}

template <typename... Args>
	requires(sizeof...(Args) == N - 4)
inline constexpr vec(const vec<T, 2>& v1, vec<T, 2>&& v2, Args&&... args)
	: data{std::move(v1.data[0]), std::move(v1.data[1]), std::move(v2.data[0]), std::move(v2.data[1]),
		   std::forward<Args>(args)...} {}

template <class U>
	requires std::constructible_from<T, U>
inline constexpr vec(vec<U, N>&& other) noexcept {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		(((T&)data[I] = std::move(other.data[I])), ...);
	}(std::make_index_sequence<N>{});
}

template <class U>
	requires std::constructible_from<T, U>
inline constexpr vec(const vec<U, N>& other) {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		(((T&)data[I] = other.data[I]), ...);
	}(std::make_index_sequence<N>{});
}

template <class U>
inline constexpr vec& operator=(const vec<U, N>& other) {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		((static_cast<value_type&>(data[I]) = (T&)other.data[I]), ...);
	}(std::make_index_sequence<N>{});
	return *this;
}

template <class U>
inline constexpr vec& operator=(vec<U, N>&& other) noexcept {
	[&]<std::size_t... I>(std::index_sequence<I...>) {
		((static_cast<value_type&>(data[I]) = std::move(other.data[I])), ...);
	}(std::make_index_sequence<N>{});
	return *this;
}

inline constexpr T&			 operator[](std::size_t index) { return data[index]; }
inline constexpr const T&	 operator[](std::size_t index) const { return data[index]; }
inline constexpr std::size_t size() const { return N; }
