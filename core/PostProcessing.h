#pragma once

#include "Utils.h"
#include "Spectrum.h"

inline float LinearToSRGB(float linear) {
	if (linear <= 0.0031308f) {
		return 12.92f * linear;
	}
	else {
		return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
	}
}

inline RGBSpectrum LinearToSRGB(const RGBSpectrum& linear) {
	RGBSpectrum srgb;
	srgb[0] = LinearToSRGB(linear[0]);
	srgb[1] = LinearToSRGB(linear[1]);
	srgb[2] = LinearToSRGB(linear[2]);

	return srgb;
}

class ToneMapper {
public:
	virtual RGBSpectrum ToneMapping(const RGBSpectrum& color) = 0;
};

class Reinhard : public ToneMapper {
public:
	Reinhard() = default;

	virtual RGBSpectrum ToneMapping(const RGBSpectrum& color) override;

public:
	static constexpr float limit = 1.5f;
};

class Uncharted2 : public ToneMapper {
public:
	Uncharted2() = default;

	virtual RGBSpectrum ToneMapping(const RGBSpectrum& color) override;

protected:
	static constexpr float a = 0.15f;
	static constexpr float b = 0.50f;
	static constexpr float c = 0.10f;
	static constexpr float d = 0.20f;
	static constexpr float e = 0.02f;
	static constexpr float f = 0.30f;
	static constexpr float white = 11.2f;
};

class ACES : public ToneMapper {
public:
	ACES() = default;

	virtual RGBSpectrum ToneMapping(const RGBSpectrum& color) override;

protected:
	static constexpr float a = 2.51f;
	static constexpr float b = 0.03f;
	static constexpr float c = 2.43f;
	static constexpr float d = 0.59f;
	static constexpr float e = 0.14f;
};

class PostProcessing {
public:
	PostProcessing(std::shared_ptr<ToneMapper> t) : toneMapper(t) {}

	RGBSpectrum GetScreenColor(const RGBSpectrum& color);

protected:
	std::shared_ptr<ToneMapper> toneMapper;
};