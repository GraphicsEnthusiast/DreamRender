#include <color.h>
#include <spectrum.h>

NAMESPACE_BEGIN(dream)

float RGBSigmoidPolynomial::operator()(float lambda) const {
	// Compute polynomial: c₂λ² + c₁λ + c₀
	const float poly = (c2 * lambda + c1) * lambda + c0;

	return Sigmoid(poly);
}

float RGBSigmoidPolynomial::MaxValue() const {
	// Check spectrum boundaries
	float max_val = std::max((*this)(360.0f), (*this)(830.0f));

	// Compute vertex position: λ = -c₁/(2c₂)
	if (0.0f != c2) {
		const float vertex_lambda = -c1 / (2.0f * c2);
		if (vertex_lambda >= 360.0f && vertex_lambda <= 830.0f) {
			max_val = std::max(max_val, (*this)(vertex_lambda));
		}
	}

	return max_val;
}

float RGBSigmoidPolynomial::Sigmoid(float x) {
	if (std::isinf(x)) {
		return x > 0.0f ? 1.0f : 0.0f;
	}
	const float denom = 2.0f * std::sqrt(1 + x * x);

	return 0.5f + x / denom;
}

// Predefined RGB to XYZ conversion matrix (D65 white point)
constexpr float RGB_TO_XYZ[3][3] = {
	{0.412453f, 0.357580f, 0.180423f},
	{0.212671f, 0.715160f, 0.072169f},
	{0.019334f, 0.119193f, 0.950227f}
};

// Predefined XYZ to RGB conversion matrix (D65 white point)  
constexpr float XYZ_TO_RGB[3][3] = {
	{ 3.240479f, -1.537150f, -0.498535f},
	{-0.969256f,  1.875991f,  0.041556f},
	{ 0.055648f, -0.204043f,  1.057311f}
};

RGBColorSpace::RGBColorSpace(const Point2f& r, const Point2f& g, const Point2f& b, std::shared_ptr<DenselySampledSpectrum> illuminant)
	: r_(r), g_(g), b_(b), illuminant_(illuminant) {
	InitializeConversionMatrices();
}

void RGBColorSpace::InitializeConversionMatrices() {
	// For SRGB with D65, use precomputed matrices for accuracy and performance
	xyz_from_rgb = Matrix3f(
		RGB_TO_XYZ[0][0], RGB_TO_XYZ[0][1], RGB_TO_XYZ[0][2],
		RGB_TO_XYZ[1][0], RGB_TO_XYZ[1][1], RGB_TO_XYZ[1][2],
		RGB_TO_XYZ[2][0], RGB_TO_XYZ[2][1], RGB_TO_XYZ[2][2]
	);

	rgb_from_xyz = Matrix3f(
		XYZ_TO_RGB[0][0], XYZ_TO_RGB[0][1], XYZ_TO_RGB[0][2],
		XYZ_TO_RGB[1][0], XYZ_TO_RGB[1][1], XYZ_TO_RGB[1][2],
		XYZ_TO_RGB[2][0], XYZ_TO_RGB[2][1], XYZ_TO_RGB[2][2]
	);

	// Compute white point from illuminant (simplified)
	w_ = Point2f(0.3127f, 0.3290f); // Standard D65 white point
}

RGBSigmoidPolynomial RGBColorSpace::ToRGBCoeffs(const RGB& rgb) const {
	// Simplified implementation - in practice, this would use a precomputed table
	// For now, return a default polynomial
	return RGBSigmoidPolynomial(0.5f, 0.5f, 0.5f);
}

RGB RGBColorSpace::ToRGB(const RGB& xyz) const {
	return rgb_from_xyz * xyz;
}

RGB RGBColorSpace::ToXYZ(const RGB& rgb) const {
	return xyz_from_rgb * rgb;
}

RGB RGBColorSpace::Luminance() const {
	return RGB(xyz_from_rgb[1][0], xyz_from_rgb[1][1], xyz_from_rgb[1][2]);
}

const RGBColorSpace* RGBColorSpace::Lookup(const Point2f& r, const Point2f& g, const Point2f& b, const Point2f& w) {
	auto CloseEnough = [](const Point2f& a, const Point2f& b) {
		return (std::abs(a.x - b.x) < 1e-3f && std::abs(a.y - b.y) < 1e-3f);
	};

	// Check against predefined color spaces
	if (CloseEnough(r, SRGB->r_) && CloseEnough(g, SRGB->g_) &&
		CloseEnough(b, SRGB->b_) && CloseEnough(w, SRGB->w_)) {
		return SRGB;
	}

	return nullptr;
}

// Static initialization of predefined color spaces
const RGBColorSpace* RGBColorSpace::SRGB = nullptr;

NAMESPACE_END(dream)