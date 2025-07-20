#pragma once

#include <fstream>
#include <img/image.hpp>
#include <stb_image_write.h>

template <class ColorFormat>
struct export_PPM {
	auto operator()(const Image<ColorFormat>& image, std::ostream& out) {
		if (!out) return false;

		out << "P3\n";
		out << image.getWidth() << ' ' << image.getHeight() << '\n';
		out << "255\n";

		for (const auto& color : image) {
			auto converted = convert<ColorFormat, RGB>(color);
			out << static_cast<int>(converted.x) << ' ' << static_cast<int>(converted.y) << ' '
				<< static_cast<int>(converted.z) << '\n';
		}

		return true;
	}
};

template <class ColorFormat>
struct export_PPM_BIN {
	auto operator()(const Image<ColorFormat>& image, std::ostream& out) {
		if (!out) return false;

		out << "P6\n";
		out << image.getWidth() << ' ' << image.getHeight() << '\n';
		out << "255\n";

		for (const auto& color : image) {
			auto converted = convert<ColorFormat, RGB>(color);
			out.write(reinterpret_cast<const char*>(&converted), sizeof(converted));
		}
		return true;
	}
};

template <class Exporter, class ColorFormat>
inline void exportToFile(const Image<ColorFormat>& img, const std::string_view& filename) {
	std::ofstream out(filename.data(), std::ios::binary);
	if (!out) { throw std::runtime_error("Failed to open file for writing: " + std::string(filename)); }
	Exporter{}(img, out);
}

class export_PNG {};

template <>
inline void exportToFile<export_PNG, RGBA>(const Image<RGBA>& img, const std::string_view& filename) {
	stbi_write_png(filename.data(), img.getWidth(), img.getHeight(), 4, img.data(), img.getWidth() * sizeof(RGBA));
}

template <>
inline void exportToFile<export_PNG, RGBA32F>(const Image<RGBA32F>& img, const std::string_view& filename) {
	uint8_t* data = new uint8_t[img.getWidth() * img.getHeight() * 4];
	for (size_t i = 0; i < img.getWidth() * img.getHeight(); ++i) {
		auto converted = convert<RGBA32F, RGBA>(img[i]);
		data[(i << 2)] = converted[0];
		data[(i << 2) + 1] = converted[1];
		data[(i << 2) + 2] = converted[2];
		data[(i << 2) + 3] = converted[3];
	}
	stbi_write_png(filename.data(), img.getWidth(), img.getHeight(), 4, data, img.getWidth() * sizeof(RGBA));
	delete[] data;
}
