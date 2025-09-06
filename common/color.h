#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

using XYZ = Vector3f;
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
    RGBSigmoidPolynomial() : c0_(0), c1_(0), c2_(0) {}

    /**
     * @brief Initialize with specific polynomial coefficients
     */
    RGBSigmoidPolynomial(float c0, float c1, float c2) : c0_(c0), c1_(c1), c2_(c2) {}

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
    // f(λ) = s(c₀λ² + c₁λ + c₂), Coefficients storage (c₀, c₁, c₂ for λ², λ, constant terms)
    float c0_, c1_, c2_;
};

/**
 * @class RGBToSpectrumTable
 * @brief Table for converting RGB values to spectral coefficients using sigmoid polynomials
 */
class RGBToSpectrumTable {
public:
    ///< Resolution of the conversion table in each dimension
    static const int res = 64;

    ///< Type definition for the coefficient array structure
    using CoefficientArray = float[3][res][res][res][4];

    /**
     * @brief Constructs a new RGB to spectrum conversion table
     * @param coeffs Pointer to the coefficient array data
     */
    RGBToSpectrumTable(const CoefficientArray* coeffs) : coeffs_(coeffs) {}

    /**
     * @brief Convert RGB color to spectral coefficients
     * @param rgb Input RGB color (components should be in [0,1] range)
     * @return RGBSigmoidPolynomial coefficients for spectral representation
     */
    RGBSigmoidPolynomial operator()(RGB rgb) const;

    /**
     * @brief Initialize the RGB to spectrum conversion tables
     */
    static void Init();

public:
    static std::unique_ptr<RGBToSpectrumTable> SRGBToSpectrumTable_; ///< Predefined sRGB to spectrum conversion table

protected:
    const CoefficientArray* coeffs_; ///< Coefficient data for RGB to spectrum conversion
};

// Forward declarations
class DenselySampledSpectrum;

/**
 * @class RGBColorSpace
 * @brief Represents an RGB color space with conversion capabilities to/from CIE XYZ(only support SRGB)
 */
class RGBColorSpace {
    friend class RGBIlluminantSpectrum;

public:
    /**
     * @brief Constructs an RGB color space from chromaticity coordinates(only support srgb and d65)
     */
    RGBColorSpace();

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
    RGB ToRGB(const XYZ& xyz) const;

    /**
     * @brief Converts RGB values to CIE XYZ color space
     * @param rgb Input RGB color values
     * @return XYZ color values
     */
    XYZ ToXYZ(const RGB& rgb) const;

    /**
     * @brief Retrieves the luminance for this color space
     * @return RGB representing luminance coefficients
     */
    RGB Luminance() const;

protected:
    /**
     * @brief Initializes the conversion matrices based on primary chromaticities
     */
    void InitializeConversionMatrices();

protected:
    // Color space properties
    std::shared_ptr<DenselySampledSpectrum> illuminant_;         ///< Reference illuminant spectrum
    Matrix3f xyz_from_rgb_;                                      ///< Conversion matrix from RGB to XYZ
    Matrix3f rgb_from_xyz_;                                      ///< Conversion matrix from XYZ to RGB
};

NAMESPACE_END(dream)