#pragma once

#include "Utils.h"
#include "Spectrum.h"

enum TextureType {
	ConstantTexture,
	ImageTexture,
	HdrTexture
};

struct TextureParams {
	TextureType type;
	Spectrum color;
	std::string filepath;
};

class Texture {
public:
	Texture(TextureType type) : m_type(type) {}

	virtual Spectrum GetColor(const Point2f& uv) = 0;

	virtual TextureType GetType() const {
		return m_type;
	}

	static std::shared_ptr<Texture> Create(const TextureParams& params);

protected:
	TextureType m_type;
};

class Constant : public Texture {
public:
	Constant(const Spectrum& c) : Texture(TextureType::ConstantTexture), color(c) {}

	virtual Spectrum GetColor(const Point2f& uv) override;

private:
	Spectrum color;
};

class Image : public Texture {
public:
	Image(const std::string& filepath);

	~Image();

	virtual Spectrum GetColor(const Point2f& uv) override;

private:
	unsigned char* data;
	int nx, ny, nn;
};

class Hdr : public Texture {
	friend InfiniteArea;

public:
	Hdr(const std::string& filepath);

	~Hdr();

	virtual Spectrum GetColor(const Point2f& uv) override;

private:
	float* data;
	int nx, ny, nn;
};