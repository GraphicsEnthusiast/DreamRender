#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

using RGB = Vector3f;

/**
 * @class RGBSigmoidPolynomial
 * @brief Represents a sigmoid polynomial for RGB spectrum modeling
 */
class RGBSigmoidPolynomial {
public:
    /**
     * @brief Default constructor (zero-initialize coefficients)
     */ 
    RGBSigmoidPolynomial() : c2(0), c1(0), c0(0) {}

    /**
     * @brief Initialize with specific polynomial coefficients
     */
    RGBSigmoidPolynomial(float c0, float c1, float c2) : c2(c2), c1(c1), c0(c0) {}

    /**
     * @brief Evaluate polynomial at given wavelength
     * @param lambda Wavelength in nanometers (typically 360-830nm)
     * @return Normalized sigmoid output in [0,1] range
     */
    float operator()(float lambda) const;

    /**
     * @brief Compute maximum value in visible spectrum (360-830nm)
     * @return Peak response value within valid wavelength range
     */
    float MaxValue() const;

protected:
    /**
     * @brief Sigmoid normalization function
     * @param x Input polynomial value
     * @return Value mapped to [0,1] via s(x) = 0.5 + x/(2√(1+x²))
     */
    static float Sigmoid(float x);

protected:
	// f(λ) = s(c₂λ² + c₁λ + c₀), Coefficients storage (c₂, c₁, c₀ for λ², λ, constant terms)
	float c2, c1, c0;
};

/**
 * @brief Evaluates polynomial using Horner's method
 * @tparam T Floating-point type
 * @param t Input value
 * @param coefficients Polynomial coefficients (highest degree first)
 * @return Σ coefficients[i]·tⁱ (Example: EvaluatePolynomial(x, {c2, c1, c0}) = c₂x² + c₁x + c₀)
 */
template <typename T, typename... Coeffs>
constexpr T EvaluatePolynomial(T t, Coeffs... coefficients) {
    const T coeffs[] = { static_cast<T>(coefficients)... };
    T result = 0;
    for (auto c : coeffs) {
        result = result * t + c;
    }

    return result;
}

// Forward declarations
class DenselySampledSpectrum;

/**
 * @class RGBColorSpace
 * @brief Represents an RGB color space with conversion capabilities to/from CIE XYZ
 */
class RGBColorSpace {
public:
    /**
     * @brief Constructs an RGB color space from chromaticity coordinates
     * @param r Red primary chromaticity coordinates (x, y)
     * @param g Green primary chromaticity coordinates (x, y)
     * @param b Blue primary chromaticity coordinates (x, y)
     * @param illuminant Reference illuminant spectrum (e.g., D65)
     */
    RGBColorSpace(const Point2f& r, const Point2f& g, const Point2f& b, std::shared_ptr<DenselySampledSpectrum> illuminant);

    /**
     * @brief Converts RGB values to spectral coefficients using sigmoid polynomials
     * @param rgb Input RGB color values
     * @return RGBSigmoidPolynomial coefficients for spectral representation
     */
    RGBSigmoidPolynomial ToRGBCoeffs(const RGB& rgb) const;

    /**
     * @brief Converts CIE XYZ values to RGB color space
     * @param xyz Input XYZ color values
     * @return RGB color values in this color space
     */
    RGB ToRGB(const RGB& xyz) const;

    /**
     * @brief Converts RGB values to CIE XYZ color space
     * @param rgb Input RGB color values
     * @return XYZ color values
     */
    RGB ToXYZ(const RGB& rgb) const;

    /**
     * @brief Retrieves the luminance for this color space
     * @return RGB representing luminance coefficients
     */
    RGB Luminance() const;

    /**
     * @brief Finds a color space matching the given chromaticities
     * @param r Red primary chromaticity
     * @param g Green primary chromaticity
     * @param b Blue primary chromaticity
     * @param w White point chromaticity
     * @return Matching color space or nullptr if no match found
     */
    static const RGBColorSpace* Lookup(const Point2f& r, const Point2f& g, const Point2f& b, const Point2f& w);

    // Predefined color space instances
    static const RGBColorSpace* SRGB;

    // Color space properties
    Point2f r_, g_, b_, w_;                                     ///< Primary and white point chromaticities
    std::shared_ptr<DenselySampledSpectrum> illuminant_;        ///< Reference illuminant spectrum
    Matrix3f xyz_from_rgb;                                      ///< Conversion matrix from RGB to XYZ
    Matrix3f rgb_from_xyz;                                      ///< Conversion matrix from XYZ to RGB

protected:
    /**
     * @brief Initializes the conversion matrices based on primary chromaticities
     */
    void InitializeConversionMatrices();
};

NAMESPACE_END(dream)