#pragma once

#include <Utils.h>

class Texture {
public:
	virtual vec3 Value(const vec2& uv) = 0;
};

class ConstantTexture : public Texture {
public:
	ConstantTexture(vec3 c) : color(c) {}

	virtual vec3 Value(const vec2& uv) override;

public:
	vec3 color;
};

class ImageTexture : public Texture {
public:
	ImageTexture(const char* filename);
	~ImageTexture();

	virtual vec3 Value(const vec2& uv) override;

public:
	unsigned char* data;
	int nx, ny, nn;
};

class HdrTexture : public Texture {
public:
	HdrTexture(const char* filename);
	~HdrTexture();

	virtual vec3 Value(const vec2& uv) override;

public:
	float* data;
	int nx, ny, nn;
};