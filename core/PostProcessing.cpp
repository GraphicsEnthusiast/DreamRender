#include "PostProcessing.h"

RGBSpectrum Reinhard::ToneMapping(const RGBSpectrum& color) {
	float luminance = Luminance(color);

	return color / (1.0f + luminance / limit);
}

RGBSpectrum Uncharted2::ToneMapping(const RGBSpectrum& color) {
	auto F = [](const RGBSpectrum &x) noexcept {
		return (x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f) - e / f;
	};

	return F(1.6f * color) / F(white);
}

RGBSpectrum ACES::ToneMapping(const RGBSpectrum& color) {
	return (color * (a * color + b)) / (color * (c * color + d) + e);
}

RGBSpectrum PostProcessing::GetScreenColor(const RGBSpectrum& color) {
	RGBSpectrum screenColor = LinearToSRGB(toneMapper->ToneMapping(color));

	return screenColor;
}
