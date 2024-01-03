#pragma once

#include "Utils.h"
#include "Spectrum.h"

enum TextureType {
	ConstantTexture,
	ImageTexture,
	HdrTexture
};

class Texture {
public:
	Texture(TextureType type) : m_type(type) {}

	virtual RGBSpectrum GetColor(const Point2f& uv) = 0;

	virtual TextureType GetType() const {
		return m_type;
	}

protected:
	TextureType m_type;
};

class Constant : public Texture {
public:
	Constant(const RGBSpectrum& c) : Texture(TextureType::ConstantTexture), color(c) {}

	virtual RGBSpectrum GetColor(const Point2f& uv) override;

private:
	RGBSpectrum color;
};

class Image : public Texture {
public:
	Image(const std::string& filepath);

	~Image();

	virtual RGBSpectrum GetColor(const Point2f& uv) override;

private:
	unsigned char* data;
	int nx, ny, nn;
};

class Hdr : public Texture {
	friend InfiniteArea;

public:
	Hdr(const std::string& filepath);

	~Hdr();

	virtual RGBSpectrum GetColor(const Point2f& uv) override;

private:
	float* data;
	int nx, ny, nn;
};