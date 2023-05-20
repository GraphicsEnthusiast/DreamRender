#include <Texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

vec3 ConstantTexture::Value(const vec2& uv, const vec3& p) {
	return color;
}

vec3 CheckerTexture::Value(const vec2& uv, const vec3& p) {
	float sines = sin(10.0f * p.x) * sin(10.0f * p.y) * sin(10.0f * p.z);
	if (sines < 0.0f) {
		return odd->Value(uv, p);
	}
	else {
		return even->Value(uv, p);
	}
}

ImageTexture::ImageTexture(const char* filename) {
	data = stbi_load(filename, &nx, &ny, &nn, 0);
	if (data == NULL) {
		cerr << "texture is null.\n";
	}
}

ImageTexture::~ImageTexture() {
	if (data != NULL) {
		delete[] data;
		data = NULL;
	}
}

vec3 ImageTexture::Value(const vec2& uv, const vec3& p) {
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

	return vec3(r, g, b);
}

HdrTexture::HdrTexture(const char* filename) {
	data = stbi_loadf(filename, &nx, &ny, &nn, 0);
	if (data == NULL) {
		cerr << "texture is null.\n";
	}
}

HdrTexture::~HdrTexture() {
	if (data != NULL) {
		delete[] data;
		data = NULL;
	}
}

vec3 HdrTexture::Value(const vec2& uv, const vec3& p) {
	if (data == NULL) {
		cerr << "texture is null.\n";
	}

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

	return vec3(r, g, b);
}