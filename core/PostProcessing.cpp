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
	RGBSpectrum screenColor = LinearToSRGB(toneMapper->ToneMapping(color * std::exp2(exposure)));

	return screenColor;
}

std::shared_ptr<ToneMapper> ToneMapper::Create(const ToneMapperParams& params) {
	if (params.type == ToneMapperType::ReinhardToneMapper) {
		return std::make_shared<Reinhard>();
	}
	else if (params.type == ToneMapperType::Uncharted2ToneMapper) {
		return std::make_shared<Uncharted2>();
	}
	else if (params.type == ToneMapperType::ACESToneMapper) {
		return std::make_shared<ACES>();
	}

	return NULL;
}

std::shared_ptr<PostProcessing> PostProcessing::Create(const PostProcessingParams& params) {
	return std::make_shared<PostProcessing>(params.toneMapper, params.exposure);
}