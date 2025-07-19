#pragma once

#include <myglm/myglm.h>
#include <ranges>
#include <json/json.hpp>
#include <lib/stb_image.h>

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
	Image() : Base(), width(100), height(100) {}
	Image(std::size_t w, std::size_t h) : Base(w * h), width(w), height(h) {}
	Image(const JSONObject& obj) {
		width  = obj["width"].as<JSONNumber>();
		height = obj["height"].as<JSONNumber>();
		Base::resize(width * height);
	}

	using Base::operator[];
	inline constexpr auto&		 operator()(std::size_t x, std::size_t y) { return Base::operator[](y* width + x); }
	inline constexpr const auto& operator()(std::size_t x, std::size_t y) const {
		return Base::operator[](y* width + x);
	}

	using Base::begin;
	using Base::end;
	using Base::size;

	inline constexpr std::size_t getWidth() const { return width; }
	inline constexpr std::size_t getHeight() const { return height; }
	inline constexpr ivec2		 resolution() const { return ivec2(static_cast<int>(width), static_cast<int>(height)); }

	inline constexpr auto Iterate() const {
		return std::views::cartesian_product(std::views::iota(0u, width), std::views::iota(0u, height));
	}

	inline void resize(std::size_t w, std::size_t h) {
		width  = w;
		height = h;
		Base::resize(width * height);
	}

	void loadFromFile(const std::string& filename) {
		stbi_set_flip_vertically_on_load(1);
		unsigned char* data = stbi_load(filename.c_str(), (int*)&width, (int*)&height, nullptr, 4);
		if (!data) { throw std::runtime_error("Failed to load image from file: " + filename); }
		this->resize(width, height);
		for (std::size_t i = 0; i < width * height; ++i) {
			RGBA color{data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]};
			this->operator[](i) = convert<RGBA, ColorFormat>(color);
		}

		stbi_image_free(data);
		stbi_set_flip_vertically_on_load(0);
	}

	ColorFormat sample(vec2 uv) const {
		uv	  = clamp(uv, 0.f, 1.f);
		int x = static_cast<int>(uv.x * width);
		int y = static_cast<int>(uv.y * height);
		auto res = this->operator()(x, y);
		return res;
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
	return RGB{static_cast<uint8_t>(std::clamp(color.x, 0.f, 1.f) * 255.0f),
			   static_cast<uint8_t>(std::clamp(color.y, 0.f, 1.f) * 255.0f),
			   static_cast<uint8_t>(std::clamp(color.z, 0.f, 1.f) * 255.0f)};
}

template <>
inline constexpr RGBA convert<RGB32F, RGBA>(const RGB32F& color) {
	return RGBA{static_cast<uint8_t>(std::clamp(color.x, 0.f, 1.f) * 255.0f),
				static_cast<uint8_t>(std::clamp(color.y, 0.f, 1.f) * 255.0f),
				static_cast<uint8_t>(std::clamp(color.z, 0.f, 1.f) * 255.0f), 255};
}

template <>
inline constexpr RGB32F convert<RGBA, RGB32F>(const RGBA& color) {
	return RGB32F{static_cast<float>(color.x) / 255.0f, static_cast<float>(color.y) / 255.0f,
				  static_cast<float>(color.z) / 255.0f};
}

template <>
inline constexpr RGBA convert<RGBA32F, RGBA>(const RGBA32F& color) {
	return RGBA{static_cast<uint8_t>(std::clamp(color.x, 0.f, 1.f) * 255.0f),
				static_cast<uint8_t>(std::clamp(color.y, 0.f, 1.f) * 255.0f),
				static_cast<uint8_t>(std::clamp(color.z, 0.f, 1.f) * 255.0f),
				static_cast<uint8_t>(std::clamp(color.w, 0.f, 1.f) * 255.0f)};
}
