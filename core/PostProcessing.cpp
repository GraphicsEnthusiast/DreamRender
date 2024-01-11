#include "PostProcessing.h"

Spectrum Reinhard::ToneMapping(const Spectrum& color) {
	float luminance = Luminance(color);

	return color / (1.0f + luminance / limit);
}

Spectrum Uncharted2::ToneMapping(const Spectrum& color) {
	auto F = [](const Spectrum &x) noexcept {
		return (x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f) - e / f;
	};

	return F(1.6f * color) / F(white);
}

Spectrum ACES::ToneMapping(const Spectrum& color) {
	return (color * (a * color + b)) / (color * (c * color + d) + e);
}

Spectrum PostProcessing::GetScreenColor(const Spectrum& color) {
	Spectrum screenColor = LinearToSRGB(toneMapper->ToneMapping(color * std::exp2(exposure)));

	return screenColor;
}
