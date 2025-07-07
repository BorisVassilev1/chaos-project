#pragma once

#include <myglm/myglm.h>
#include <ranges>

using RGB	  = vec<uint8_t, 3>;
using RGBA	  = vec<uint8_t, 4>;
using RGB32F  = vec<float, 3>;
using RGBA32F = vec<float, 4>;

template <class ColorFormat>
class Image : public std::vector<ColorFormat> {
	std::size_t width;
	std::size_t height;

	using Base = std::vector<ColorFormat>;

   public:
	Image(std::size_t w, std::size_t h) : Base(w * h), width(w), height(h) {}

	using Base::operator[];
	inline constexpr auto& operator()(std::size_t x, std::size_t y) { return Base::operator[](y* width + x); }

	using Base::begin;
	using Base::end;
	using Base::size;

	inline constexpr std::size_t getWidth() const { return width; }
	inline constexpr std::size_t getHeight() const { return height; }
	inline constexpr ivec2 resolution() const {
		return ivec2(static_cast<int>(width), static_cast<int>(height));
	}

	inline constexpr auto Iterate() const {
		return std::views::cartesian_product(std::views::iota(0u, width), std::views::iota(0u, height));
	}
};

template <class ColorFormat1, class ColorFormat2>
inline constexpr auto convert(const ColorFormat1& color) -> ColorFormat2 {
	static_assert(std::is_same_v<ColorFormat1, ColorFormat2>,
				  "Color formats must be the same or specialized for conversion.");
	return color;
}

template <>
inline constexpr RGBA convert<RGB, RGBA>(const RGB& color) {
	return RGBA{color.x, color.y, color.z, (uint8_t)255};
}
template <>
inline constexpr RGB convert<RGBA, RGB>(const RGBA& color) {
	return RGB{color.x, color.y, color.z};
}
template <>
inline constexpr RGBA32F convert<RGB32F, RGBA32F>(const RGB32F& color) {
	return RGBA32F{color.x, color.y, color.z, 1.0f};
}
template <>
inline constexpr RGB32F convert<RGBA32F, RGB32F>(const RGBA32F& color) {
	return RGB32F{color.x, color.y, color.z};
}
template <>
inline constexpr RGB32F convert<RGB, RGB32F>(const RGB& color) {
	return RGB32F{static_cast<float>(color.x) / 255.0f, static_cast<float>(color.y) / 255.0f,
				  static_cast<float>(color.z) / 255.0f};
}
template <>
inline constexpr RGB convert<RGB32F, RGB>(const RGB32F& color) {
	return RGB{static_cast<uint8_t>(color.x * 255.0f), static_cast<uint8_t>(color.y * 255.0f),
			   static_cast<uint8_t>(color.z * 255.0f)};
}
