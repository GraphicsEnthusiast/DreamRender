#pragma once

#include <Utils.h>

class Texture {
public:
	virtual vec3 Value(const vec2& uv, const vec3& p) = 0;
};

class ConstantTexture : public Texture {
public:
	ConstantTexture(vec3 c) : color(c) {}

	virtual vec3 Value(const vec2& uv, const vec3& p) override;

public:
	vec3 color;
};

class CheckerTexture : public Texture {
public:
    CheckerTexture(shared_ptr<Texture> t0, shared_ptr<Texture> t1): even(t0), odd(t1) {}

    virtual vec3 Value(const vec2& uv, const vec3& p) override;

public:
	shared_ptr<Texture> odd;
	shared_ptr<Texture> even;
};

class ImageTexture : public Texture {
public:
	ImageTexture(const char* filename);
	~ImageTexture();

	virtual vec3 Value(const vec2& uv, const vec3& p) override;

public:
	unsigned char* data;
	int nx, ny, nn;
};

class HdrTexture : public Texture {
public:
	HdrTexture(const char* filename);
	~HdrTexture();

	virtual vec3 Value(const vec2& uv, const vec3& p) override;

public:
	float* data;
	int nx, ny, nn;
};