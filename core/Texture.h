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

	inline virtual int GetWidth() const {
		return 0;
	}

	inline virtual int GetHeight() const {
		return 0;
	}

	inline virtual TextureType GetType() const {
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
	Image(const std::string& filename);

	~Image();

	virtual RGBSpectrum GetColor(const Point2f& uv) override;

	inline virtual int GetWidth() const override {
		return nx;
	}

	inline virtual int GetHeight() const override {
		return ny;
	}

private:
	unsigned char* data;
	int nx, ny, nn;
};

class Hdr : public Texture {
public:
	Hdr(const std::string& filename);

	~Hdr();

	virtual RGBSpectrum GetColor(const Point2f& uv) override;

	inline virtual int GetWidth() const override {
		return nx;
	}

	inline virtual int GetHeight() const override {
		return ny;
	}

private:
	float* data;
	int nx, ny, nn;
};