#pragma once

#include <fstream>
#include <img/image.hpp>

template <class ColorFormat>
auto export_PPM(const Image<ColorFormat>& image, std::ostream& out) {
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

template <class ColorFormat>
auto export_PPM_BIN(const Image<ColorFormat>& image, std::ostream& out) {
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

template <class Exporter, class ColorFormat>
inline void exportToFile(const Image<ColorFormat> &img, const std::string_view &filename, Exporter && exp) {
	std::ofstream out(filename.data(), std::ios::binary);
	if (!out) {
		throw std::runtime_error("Failed to open file for writing: " + std::string(filename));
	}
	exp(img, out);
}
