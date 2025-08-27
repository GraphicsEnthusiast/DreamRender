#pragma once

#include <color.h>

NAMESPACE_BEGIN(dream)

/**
 * @class DenselySampledSpectrum
 * @brief Represents a densely sampled spectral power distribution
 */
class DenselySampledSpectrum {
public:
    /**
     * @brief Constructs an empty spectrum with given wavelength bounds
     * @param lambda_min Minimum wavelength (default: 360nm)
     * @param lambda_max Maximum wavelength (default: 830nm)
     */
    DenselySampledSpectrum(int lambda_min = 360, int lambda_max = 830);

    /**
     * @brief Constructs by sampling a spectral function
     * @param func Spectral function to sample
     * @param lambda_min Minimum wavelength (default: 360nm)
     * @param lambda_max Maximum wavelength (default: 830nm)
     */
    template <typename Func>
    DenselySampledSpectrum(Func func, int lambda_min = 360, int lambda_max = 830);

    /**
    * @brief Evaluates the spectrum at a specific wavelength
    * @param lambda Wavelength value to evaluate
    * @return Spectral value at the given wavelength, or 0 if out of range
    */
    float operator()(float lambda) const;

    /**
     * @brief Compares two DenselySampledSpectrum objects for equality
     * @param d Other spectrum to compare with
     * @return True if both spectra have identical wavelength ranges and values
     */
    bool operator==(const DenselySampledSpectrum& d) const;

    /**
     * @brief Samples the spectrum at specified wavelengths
     * @param wavelengths Array of wavelengths to sample
     * @return Sampled spectrum values at input wavelengths
     */
    std::vector<float> Sample(const std::vector<float>& wavelengths) const;

    /**
     * @brief Scales all spectral values by a constant factor
     * @param s Scaling factor
     */
    void Scale(float s);

    /**
     * @brief Computes the maximum value in the spectrum
     * @return Peak spectral value
     */
    float MaxValue() const;

protected:
    int lambda_min_;            ///< Lower bound of wavelength range (inclusive)
    int lambda_max_;            ///< Upper bound of wavelength range (inclusive)
    std::vector<float> values_; ///< Spectral values (index = wavelength - lambda_min)
};

/**
 * @class RGBAlbedoSpectrum
 * @brief Represents reflectance spectrum derived from RGB values
 */
class RGBAlbedoSpectrum {
public:
    /**
     * @brief Constructs from RGB color in given color space
     * @param cs RGB color space definition
     * @param rgb Input RGB color (components in [0,1])
     */
    RGBAlbedoSpectrum(const RGBColorSpace& cs, const RGB& rgb);

    /**
     * @brief Evaluates spectrum at given wavelength
     * @param lambda Wavelength in nanometers
     * @return Spectral reflectance value
     */
    float operator()(float lambda) const;

    /**
     * @brief Computes maximum reflectance value in visible spectrum
     * @return Peak reflectance value
     */
    float MaxValue() const;

protected:
    RGBSigmoidPolynomial rsp_;  ///< Sigmoid polynomial coefficients
};

/**
 * @class RGBUnboundedSpectrum
 * @brief Represents unconstrained spectral distributions (e.g., emissive sources)
 */
class RGBUnboundedSpectrum {
public:
    /**
     * @brief Constructs from RGB color with intensity scaling
     * @param cs RGB color space definition
     * @param rgb Input RGB color (components >=0)
     */
    RGBUnboundedSpectrum(const RGBColorSpace& cs, const RGB& rgb);

    /**
     * @brief Evaluates scaled spectrum at wavelength
     * @param lambda Wavelength in nanometers
     * @return Spectral radiance value
     */
    float operator()(float lambda) const;

    /**
     * @brief Computes maximum radiant intensity
     * @return Peak radiance value
     */
    float MaxValue() const;

protected:
    float scale_;                 ///< Intensity scaling factor
    RGBSigmoidPolynomial rsp_;    ///< Sigmoid polynomial coefficients
};

/**
 * @class RGBIlluminantSpectrum
 * @brief Represents light source spectrum combined with illuminant
 */
class RGBIlluminantSpectrum {
public:
    /**
     * @brief Constructs illuminant spectrum
     * @param cs RGB color space containing illuminant data
     * @param rgb RGB color defining spectral shape
     */
    RGBIlluminantSpectrum(const RGBColorSpace& cs, const RGB& rgb);

    /**
     * @brief Evaluates full spectral power distribution
     * @param lambda Wavelength in nanometers
     * @return Spectral radiance value
     */
    float operator()(float lambda) const;

    /**
     * @brief Computes maximum spectral radiance
     * @return Peak radiance value
     */
    float MaxValue() const;

protected:
    float scale_;                                        ///< Intensity scaling factor
    RGBSigmoidPolynomial rsp_;                           ///< Spectral shape coefficients
    std::shared_ptr<DenselySampledSpectrum> illuminant_; ///< Ptr illuminant (e.g., D65)
};

NAMESPACE_END(dream)