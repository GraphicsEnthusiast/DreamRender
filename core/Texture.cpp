#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

RGBSpectrum Constant::GetColor(const Point2f& uv) {
	return color;
}

Image::Image(const std::string& filepath) : Texture(TextureType::ImageTexture) {
	stbi_set_flip_vertically_on_load(true);
	data = stbi_load(filepath.c_str(), &nx, &ny, &nn, 0);
	if (data == NULL) {
		std::cerr << "Texture is null.\n";
		assert(0);
	}
}

Image::~Image() {
	if (data != NULL) {
		delete[] data;
		data = NULL;
	}
}

RGBSpectrum Image::GetColor(const Point2f& uv) {
	float u = uv.x;
	float v = uv.y;
	if (u > 1.0f || u < 0.0f) {
		int int_u = static_cast<int>(u);
		u = u - int_u;
		if (u < 0.0f) {
			u += 1.0f;
		}
	}
	if (v > 1.0f || v < 0.0f) {
		int int_v = static_cast<int>(v);
		v = v - int_v;
		if (v < 0.0f) {
			v += 1.0f;
		}
	}

	int i = static_cast<int>((u) * nx);
	int j = static_cast<int>((1.0f - v) * ny);

	if (i < 0) {
		i = 0;
	}
	if (j < 0) {
		j = 0;
	}
	if (i > nx - 1) {
		i = nx - 1;
	}
	if (j > ny - 1) {
		j = ny - 1;
	}

	float r = (data[nn * i + nn * nx * j + 0]) / 255.0f;
	float g = (data[nn * i + nn * nx * j + 1]) / 255.0f;
	float b = (data[nn * i + nn * nx * j + 2]) / 255.0f;
	float rgb[3] = { r, g, b };

	return RGBSpectrum::FromRGB(rgb);
}

Hdr::Hdr(const std::string& filepath) : Texture(TextureType::HdrTexture) {
	stbi_set_flip_vertically_on_load(true);
	data = stbi_loadf(filepath.c_str(), &nx, &ny, &nn, 0);
	if (data == NULL) {
		std::cerr << "Texture is null.\n";
		assert(0);
	}
}

Hdr::~Hdr() {
	if (data != NULL) {
		delete[] data;
		data = NULL;
	}
}

RGBSpectrum Hdr::GetColor(const Point2f& uv) {
	int i = static_cast<int>(uv.x * nx);
	int j = static_cast<int>(uv.y * ny);

	if (i < 0) {
		i = 0;
	}
	if (j < 0) {
		j = 0;
	}
	if (i > nx - 1) {
		i = nx - 1;
	}
	if (j > ny - 1) {
		j = ny - 1;
	}

	float r = data[nn * i + nn * nx * j + 0];
	float g = data[nn * i + nn * nx * j + 1];
	float b = data[nn * i + nn * nx * j + 2];
	float rgb[3] = { r, g, b };

	return RGBSpectrum::FromRGB(rgb);
}