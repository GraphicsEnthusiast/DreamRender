#include <spectrum.h>

NAMESPACE_BEGIN(dream)

DenselySampledSpectrum::DenselySampledSpectrum(int lambda_min, int lambda_max)
	: lambda_min_(lambda_min), lambda_max_(lambda_max),
	values_(lambda_max - lambda_min + 1, 0.0f) {}

template <typename Func>
DenselySampledSpectrum::DenselySampledSpectrum(Func func, int lambda_min, int lambda_max)
	: lambda_min_(lambda_min), lambda_max_(lambda_max),
	values_(lambda_max - lambda_min + 1) {
	for (int lambda = lambda_min; lambda <= lambda_max; ++lambda) {
		values_[lambda - lambda_min] = func(lambda);
	}
}

float DenselySampledSpectrum::operator()(float lambda) const {
	int offset = std::lround(lambda) - lambda_min_;
	if (offset < 0.0f || offset >= values_.size()) {
		return 0.0f;
	}

	return values_[offset];
}

void DenselySampledSpectrum::Scale(float s) {
	for (auto& value : values_) {
		value *= s;
	}
}

float DenselySampledSpectrum::MaxValue() const {
	return *std::max_element(values_.begin(), values_.end());
}

// RGBAlbedoSpectrum implementation
RGBAlbedoSpectrum::RGBAlbedoSpectrum(const RGBColorSpace& cs, const RGB& rgb)
	: rsp_(cs.ToRGBCoeffs(rgb)) {
	// Validate reflectance constraints (0 <= RGB <= 1)
	assert(rgb.x >= 0.0f && rgb.x <= 1.0f &&
		rgb.y >= 0.0f && rgb.y <= 1.0f &&
		rgb.z >= 0.0f && rgb.z <= 1.0f);
}

float RGBAlbedoSpectrum::operator()(float lambda) const {
	return rsp_(lambda);
}

float RGBAlbedoSpectrum::MaxValue() const {
	return rsp_.MaxValue();
}

RGBUnboundedSpectrum::RGBUnboundedSpectrum(const RGBColorSpace& cs, const RGB& rgb) : scale_(1.0f) {
	// Compute the maximum component of the RGB vector
	float max_comp = std::max({ rgb.x, rgb.y, rgb.z });
	scale_ = 2.0f * max_comp;

	// Normalize the RGB vector by scale_ if not zero, otherwise use zero
	RGB scaled_rgb = (scale_ > 0.0f) ? (rgb / scale_) : RGB(0.0f);
	rsp_ = cs.ToRGBCoeffs(scaled_rgb);
}

float RGBUnboundedSpectrum::operator()(float lambda) const {
	return scale_ * rsp_(lambda);
}

float RGBUnboundedSpectrum::MaxValue() const {
	return scale_ * rsp_.MaxValue();
}

// RGBIlluminantSpectrum implementation
RGBIlluminantSpectrum::RGBIlluminantSpectrum(const RGBColorSpace& cs, const RGB& rgb) : scale_(1.0f), illuminant_(cs.illuminant_) {
	float max_comp = std::max({ rgb.x, rgb.y, rgb.z });
	scale_ = 2.0f * max_comp;

	RGB scaled_rgb = (scale_ > 0.0f) ? (rgb / scale_) : RGB(0.0f);
	rsp_ = cs.ToRGBCoeffs(scaled_rgb);
}

float RGBIlluminantSpectrum::operator()(float lambda) const {
	return scale_ * rsp_(lambda) * (*illuminant_)(lambda);
}

float RGBIlluminantSpectrum::MaxValue() const {
	return scale_ * rsp_.MaxValue() * illuminant_->MaxValue();
}

NAMESPACE_END(dream)