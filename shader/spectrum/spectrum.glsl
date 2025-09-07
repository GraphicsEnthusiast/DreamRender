#ifndef _SPECTRUM__GLSL__
#define _SPECTRUM__GLSL__

#include "color/color_space.glsl"

/**
 * @struct SampledSpectrum
 * @brief Represents spectral distribution with discrete wavelength samples
 */
struct SampledSpectrum {
    float values[NSpectrumSamples];  // Spectral values at sampled wavelengths
};

/**
 * @brief Creates a constant spectrum
 * @param c Constant value for all wavelengths
 * @return New SampledSpectrum
 */
SampledSpectrum SampledSpectrumNew(const float c[NSpectrumSamples]) {
    SampledSpectrum s;
    for (int i = 0; i < NSpectrumSamples; i++) {
        s.values[i] = c[i];
    }

    return s;
}

/**
 * @brief Adds two spectra component-wise
 * @param a First spectrum operand
 * @param b Second spectrum operand
 * @return Component-wise sum of a and b
 */
SampledSpectrum SampledSpectrumAdd(SampledSpectrum a, SampledSpectrum b) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = a.values[i] + b.values[i];
    }

    return r;
}

/**
 * @brief Subtracts two spectra component-wise
 * @param a First spectrum operand (minuend)
 * @param b Second spectrum operand (subtrahend)
 * @return Component-wise difference of a and b (a - b)
 */
SampledSpectrum SampledSpectrumSub(SampledSpectrum a, SampledSpectrum b) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = a.values[i] - b.values[i];
    }

    return r;
}

/**
 * @brief Multiplies two spectra component-wise (Hadamard product)
 * @param a First spectrum operand
 * @param b Second spectrum operand
 * @return Component-wise product of a and b
 */
SampledSpectrum SampledSpectrumMul(SampledSpectrum a, SampledSpectrum b) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = a.values[i] * b.values[i];
    }

    return r;
}

/**
 * @brief Scales a spectrum by a scalar factor
 * @param s Spectrum to scale
 * @param a Scalar multiplier
 * @return Scaled spectrum
 */
SampledSpectrum SampledSpectrumMulFloat(SampledSpectrum s, float a) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = s.values[i] * a;
    }

    return r;
}

/**
 * @brief Divides two spectra component-wise with zero-division protection
 * @param a Numerator spectrum
 * @param b Denominator spectrum
 * @return Component-wise quotient a/b (returns 0 where denominator is 0)
 */
SampledSpectrum SampledSpectrumDiv(SampledSpectrum a, SampledSpectrum b) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = (b.values[i] != 0.0f) ? (a.values[i] / b.values[i]) : 0.0f;
    }

    return r;
}

/**
 * @brief Divides a spectrum by a scalar divisor
 * @param s Spectrum to divide
 * @param a Scalar divisor
 * @return Component-wise quotient s/a
 */
SampledSpectrum SampledSpectrumDivFloat(SampledSpectrum s, float a) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = s.values[i] / a;
    }

    return r;
}

/**
 * @brief Negates all components of a spectrum
 * @param s Input spectrum
 * @return Negated spectrum
 */
SampledSpectrum SampledSpectrumNegate(SampledSpectrum s) {
    SampledSpectrum r;
    for (int i = 0; i < NSpectrumSamples; i++) {
        r.values[i] = -s.values[i];
    }

    return r;
}

/**
 * @brief Checks if any spectral component is non-zero
 * @param s Spectrum to check
 * @return True if at least one component is non-zero, false otherwise
 */
bool SampledSpectrumNonZero(SampledSpectrum s) {
    for (int i = 0; i < NSpectrumSamples; i++) {
        if (0.0f != s.values[i]) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Finds the minimum value across all spectral components
 * @param s Input spectrum
 * @return Minimum component value
 */
float SampledSpectrumMin(SampledSpectrum s) {
    float m = s.values[0];
    for (int i = 1; i < NSpectrumSamples; i++) {
        m = min(m, s.values[i]);
    }

    return m;
}

/**
 * @brief Finds the maximum value across all spectral components
 * @param s Input spectrum
 * @return Maximum component value
 */
float SampledSpectrumMax(SampledSpectrum s) {
    float m = s.values[0];
    for (int i = 1; i < NSpectrumSamples; i++) {
        m = max(m, s.values[i]);
    }

    return m;
}

/**
 * @brief Computes the arithmetic mean of spectral components
 * @param s Input spectrum
 * @return Average value of all spectral samples
 */
float SampledSpectrumAvg(SampledSpectrum s) {
    float sum = s.values[0];
    for (int i = 1; i < NSpectrumSamples; i++) {
        sum += s.values[i];
    }

    return sum / float(NSpectrumSamples);
}

/**
 * @struct SampledWavelengths
 * @brief Stores sampled wavelengths and their probability densities
 */
struct SampledWavelengths {
    float lambda[NSpectrumSamples];  // Sampled wavelengths
    float pdf[NSpectrumSamples];     // Probability density for each wavelength
};

/**
 * @brief Checks if two SampledWavelengths instances are equal
 * @param a First instance
 * @param b Second instance
 */
bool SampledWavelengthsEqual(SampledWavelengths a, SampledWavelengths b) {
    for (int i = 0; i < NSpectrumSamples; i++) {
        if (a.lambda[i] != b.lambda[i] || a.pdf[i] != b.pdf[i]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks if two SampledWavelengths instances are not equal
 * @param a First instance
 * @param b Second instance
 */
bool SampledWavelengthsNotEqual(SampledWavelengths a, SampledWavelengths b) {
    return !SampledWavelengthsEqual(a, b);
}

/**
 * @brief Samples wavelengths uniformly across the visible spectrum
 * @param u Random seed value [0, 1)
 * @param lambda_min Minimum wavelength (default: LambdaMin)
 * @param lambda_max Maximum wavelength (default: LambdaMax)
 */
SampledWavelengths SampledWavelengthsSampleUniform(float u, float lambda_min, float lambda_max) {
    SampledWavelengths swl;
    
    // Sample first wavelength using u
    swl.lambda[0] = mix(lambda_min, lambda_max, u);
    
    // Initialize wavelengths for remaining samples
    float delta = (lambda_max - lambda_min) / float(NSpectrumSamples);
    for (int i = 1; i < NSpectrumSamples; i++) {
        swl.lambda[i] = swl.lambda[i - 1] + delta;
        if (swl.lambda[i] > lambda_max) {
            swl.lambda[i] = lambda_min + (swl.lambda[i] - lambda_max);
        }
    }
    
    // Compute uniform PDF
    float pdf_val = 1.0f / (lambda_max - lambda_min);
    for (int i = 0; i < NSpectrumSamples; i++) {
        swl.pdf[i] = pdf_val;
    }
    
    return swl;
}

/**
 * @brief Computes wavelength using inverse CDF of visible spectrum
 * @param u Random value [0, 1)
 * @return Wavelength in nanometers
 */
float SampleVisibleWavelengths(float u) {
    // Inverse CDF approximation for visible spectrum (360-830nm)
    return 538.0f - 138.888889f * atanh(0.85691062f - 1.82750197f * u);
}

/**
 * @brief Computes PDF for given wavelength in visible spectrum
 * @param lambda Query wavelength in nanometers
 * @return Probability density value
 */
float VisibleWavelengthsPDF(float lambda) {
    // Return 0 for out-of-range wavelengths
    if (lambda < LambdaMin || lambda > LambdaMax) {
        return 0.0f;
    }
    
    // PDF formula for visible wavelengths
    float x = 0.0072f * (lambda - 538.0f);
    float cosh_x = cosh(x);

    return 0.0039398042f / (cosh_x * cosh_x);
}

/**
 * @brief Samples visible wavelengths using importance sampling
 * @param u Random seed value [0, 1)
 * @return SampledWavelengths with wavelengths and PDFs
 */
SampledWavelengths SampledWavelengthsSampleVisible(float u) {
    SampledWavelengths swl;
    
    for (int i = 0; i < NSpectrumSamples; ++i) {
        // Compute offset for i-th wavelength sample
        float up = u + float(i) / float(NSpectrumSamples);
        if (up > 1.0f) {
            up -= 1.0f;
        }
        
        // Sample wavelength and compute PDF
        swl.lambda[i] = SampleVisibleWavelengths(up);
        swl.pdf[i] = VisibleWavelengthsPDF(swl.lambda[i]);
    }
    
    return swl;
}

/**
 • @brief Terminates secondary wavelengths (keeps only primary)
 • @param swl Input SampledWavelengths instance
 • @return Modified SampledWavelengths instance
 */
SampledWavelengths SampledWavelengthsTerminateSecondary(SampledWavelengths swl) {
    SampledWavelengths result = swl;
    
    bool terminated = true;
    for (int i = 1; i < NSpectrumSamples; i++) {
        if (0.0f != result.pdf[i]) {
            terminated = false;
            break;
        }
    }
    if (terminated) {
        return result;
    }
    
    // Update probabilities
    for (int i = 1; i < NSpectrumSamples; i++) {
        result.pdf[i] = 0.0f;
    }
    result.pdf[0] /= float(NSpectrumSamples);
    
    return result;
}

/**
 * @brief Checks if secondary wavelengths have been terminated
 * @param swl SampledWavelengths instance
 */
bool SampledWavelengthsSecondaryTerminated(SampledWavelengths swl) {
    for (int i = 1; i < NSpectrumSamples; i++) {
        if (0.0f != swl.pdf[i]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Gets PDF values as SampledSpectrum
 * @param swl SampledWavelengths instance
 */
SampledSpectrum SampledWavelengthsPDF(SampledWavelengths swl) {
    SampledSpectrum s;
    for (int i = 0; i < NSpectrumSamples; i++) {
        s.values[i] = swl.pdf[i];
    }

    return s;
}

/**
 * @struct DenselySampledSpectrum
 * @brief Stores densely sampled spectral data (1nm intervals)
 */
struct DenselySampledSpectrum {
    float values[NCIESamples];  // Spectral value array
};

/**
 * @brief Creates a zero-initialized densely sampled spectrum
 * @return Initialized spectrum struct
 */
DenselySampledSpectrum DenselySampledSpectrumNew() {
    DenselySampledSpectrum s;
    for (int i = 0; i < NCIESamples; i++) {
        s.values[i] = 0.0f;
    }

    return s;
}

/**
 * @brief Samples spectral values at discrete wavelengths
 * @param d Input densely sampled spectrum
 * @param lambda Target wavelength samples
 * @return SampledSpectrum values at sampled wavelengths
 */
SampledSpectrum DenselySampledSpectrumSample(DenselySampledSpectrum d, SampledWavelengths lambda) {
    SampledSpectrum s;
    for (int i = 0; i < NSpectrumSamples; i++) {
        // Calculate array index (rounded to nearest integer)
        int idx = int(round(lambda.lambda[i]) - int(LambdaMin));
        
        if (idx < 0 || idx >= NCIESamples) {
            s.values[i] = 0.0f;  // Return 0 for out-of-range wavelengths
        } 
        else {
            s.values[i] = d.values[idx];
        }
    }

    return s;
}

/**
 * @brief Samples CIE spectral values at specified wavelengths
 * @param cie CIE spectral data array (size must be NCIESamples)
 * @param lambda Target wavelength samples with PDF values
 * @return SampledSpectrum containing values at queried wavelengths
 */
SampledSpectrum CIEDenselySampledSpectrumSample(const float cie[NCIESamples], SampledWavelengths lambda) {
    SampledSpectrum s;
    for (int i = 0; i < NSpectrumSamples; i++) {
        // Calculate array index (rounded to nearest integer)
        int idx = int(round(lambda.lambda[i]) - int(LambdaMin));
        
        if (idx < 0 || idx >= NCIESamples) {
            s.values[i] = 0.0f;  // Return 0 for out-of-range wavelengths
        } 
        else {
            s.values[i] = cie[idx];
        }
    }

    return s;
}

/**
 • @brief Scales spectral values by a factor
 • @param d Input spectrum
 • @param scale Scaling factor
 • @return Scaled spectrum
 */
DenselySampledSpectrum DenselySampledSpectrumScale(DenselySampledSpectrum d, float scale) {
    DenselySampledSpectrum result = d;
    for (int i = 0; i < NCIESamples; i++) {
        result.values[i] *= scale;
    }

    return result;
}

/**
 * @brief Finds maximum spectral value
 * @param d Input spectrum
 * @return Maximum spectral value
 */
float DenselySampledSpectrumMaxValue(DenselySampledSpectrum d) {
    float max_val = d.values[0];
    for (int i = 1; i < NCIESamples; i++) {
        max_val = max(max_val, d.values[i]);
    }

    return max_val;
}

/**
 * @brief Evaluates spectrum at specified wavelength
 * @param d Input spectrum
 * @param lambda Query wavelength
 * @return Spectral value (0 if wavelength out-of-range)
 */
float DenselySampledSpectrumEval(DenselySampledSpectrum d, float lambda) {
    int idx = int(round(lambda) - int(LambdaMin));
    if (idx < 0 || idx >= NCIESamples) {
        return 0.0f;
    }
    
    return d.values[idx];
}

/**
 * @brief Checks spectral equality
 * @param a First spectrum
 * @param b Second spectrum
 * @return True if all spectral values match
 */
bool DenselySampledSpectrumEqual(DenselySampledSpectrum a, DenselySampledSpectrum b) {
    for (int i = 0; i < NCIESamples; i++) {
        if (a.values[i] != b.values[i]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Safely divides two floats with zero-division protection
 * @param a Dividend
 * @param b Divisor
 * @return Division result (returns 0 when divisor is 0)
 */
float SafeDiv(float a, float b) {
    return (0.0f != b) ? (a / b) : 0.0f;
}

/**
 * @brief Converts a sampled spectrum to XYZ color using Monte Carlo integration
 * @param s Input sampled spectrum (NSpectrumSamples components)
 * @param lambda Sampled wavelengths with PDF values
 * @return XYZ color in CIE 1931 space
 */
XYZ SampledSpectrumToXYZ(SampledSpectrum s, SampledWavelengths lambda) {
    // Sample CIE matching functions at given wavelengths
    SampledSpectrum xs = CIEDenselySampledSpectrumSample(CIEX, lambda);
    SampledSpectrum ys = CIEDenselySampledSpectrumSample(CIEY, lambda);
    SampledSpectrum zs = CIEDenselySampledSpectrumSample(CIEZ, lambda);
    
    // Retrieve PDF spectrum for importance sampling
    SampledSpectrum pdf = SampledWavelengthsPDF(lambda);
    
    // Compute weighted averages with PDF normalization
    SampledSpectrum x_avg;
    SampledSpectrum y_avg;
    SampledSpectrum z_avg;
    
    for (int i = 0; i < NSpectrumSamples; i++) {
        x_avg.values[i] = SafeDiv(xs.values[i] * s.values[i], pdf.values[i]);
        y_avg.values[i] = SafeDiv(ys.values[i] * s.values[i], pdf.values[i]);
        z_avg.values[i] = SafeDiv(zs.values[i] * s.values[i], pdf.values[i]);
    }
    
    // Compute averages and normalize by CIE Y integral
    float x = SampledSpectrumAvg(x_avg) / CIEYIntegral;
    float y = SampledSpectrumAvg(y_avg) / CIEYIntegral;
    float z = SampledSpectrumAvg(z_avg) / CIEYIntegral;
    
    return XYZNew(x, y, z);
}

/**
 * @brief Computes luminance (Y component) of a sampled spectrum
 * @param s Input sampled spectrum
 * @param lambda Sampled wavelengths with PDF values
 * @return Luminance value (CIE Y component)
 */
float SampledSpectrumY(SampledSpectrum s, SampledWavelengths lambda) {
    // Sample CIE Y matching function
    SampledSpectrum ys = CIEDenselySampledSpectrumSample(CIEY, lambda);
    
    // Retrieve PDF spectrum
    SampledSpectrum pdf = SampledWavelengthsPDF(lambda);
    
    // Compute weighted average with PDF normalization
    float sumy = 0.0f;
    for (int i = 0; i < NSpectrumSamples; i++) {
        sumy += SafeDiv(ys.values[i] * s.values[i], pdf.values[i]);
    }
    
    // Normalize by sample count and CIE Y integral
    return (sumy / float(NSpectrumSamples)) / CIEYIntegral;
}

/**
 * @brief Converts a sampled spectrum to RGB via XYZ intermediate
 * @param s Input sampled spectrum
 * @param lambda Sampled wavelengths with PDF values
 * @return Linear RGB color
 */
RGB SampledSpectrumToRGB(SampledSpectrum s, SampledWavelengths lambda) {
    // Convert through XYZ color space
    XYZ xyz = SampledSpectrumToXYZ(s, lambda);

    return XYZToRGB(xyz);
}

/**
 * @struct RGBAlbedoSpectrum
 * @brief Represents reflectance spectrum derived from RGB values
 */
struct RGBAlbedoSpectrum {
    RGBSigmoidPolynomial rsp;  ///< Sigmoid polynomial coefficients
};

/**
 * @brief Creates an albedo spectrum from RGB coefficients
 * @param rsp Sigmoid polynomial coefficients
 * @return Initialized albedo spectrum
 */
RGBAlbedoSpectrum RGBAlbedoSpectrumNew(RGBSigmoidPolynomial rsp) {
    RGBAlbedoSpectrum s;
    s.rsp = rsp;

    return s;
}

/**
 * @brief Evaluates spectrum at given wavelength
 * @param s Input albedo spectrum
 * @param lambda Wavelength in nanometers
 * @return Spectral reflectance value
 */
float RGBAlbedoSpectrumEval(RGBAlbedoSpectrum s, float lambda) {
    return RGBSigmoidPolynomialEval(s.rsp, lambda);
}

/**
 * @brief Computes maximum reflectance value in visible spectrum
 * @param s Input albedo spectrum
 * @return Peak reflectance value
 */
float RGBAlbedoSpectrumMaxValue(RGBAlbedoSpectrum s) {
    return RGBSigmoidPolynomialMaxValue(s.rsp);
}

/**
 * @struct RGBUnboundedSpectrum
 * @brief Represents unconstrained spectral distributions (e.g., emissive sources)
 */
struct RGBUnboundedSpectrum {
    float scale;                ///< Intensity scaling factor
    RGBSigmoidPolynomial rsp;   ///< Sigmoid polynomial coefficients
};

/**
 * @brief Creates an unbounded spectrum from RGB color
 * @param cs RGB color space definition
 * @param rgb Input RGB color (components >=0)
 * @return Initialized unbounded spectrum
 */
RGBUnboundedSpectrum RGBUnboundedSpectrumNew(RGB rgb) {
    RGBUnboundedSpectrum s;
    
    // Compute the maximum component of the RGB vector
    float max_comp = max(rgb.r, max(rgb.g, rgb.b));
    s.scale = 2.0f * max_comp;
    
    // Normalize the RGB vector by scale if not zero
    RGB scaled_rgb = s.scale > 0.0f ? RGBDivFloat(rgb, s.scale) : RGBNew(0.0f, 0.0f, 0.0f);
    s.rsp = ToRGBCoeffs(scaled_rgb);
    
    return s;
}

/**
 * @brief Evaluates scaled spectrum at wavelength
 * @param s Input unbounded spectrum
 * @param lambda Wavelength in nanometers
 * @return Spectral radiance value
 */
float RGBUnboundedSpectrumEval(RGBUnboundedSpectrum s, float lambda) {
    return s.scale * RGBSigmoidPolynomialEval(s.rsp, lambda);
}

/**
 * @brief Computes maximum radiant intensity
 * @param s Input unbounded spectrum
 * @return Peak radiance value
 */
float RGBUnboundedSpectrumMaxValue(RGBUnboundedSpectrum s) {
    return s.scale * RGBSigmoidPolynomialMaxValue(s.rsp);
}

/**
 * @struct RGBIlluminantSpectrum
 * @brief Represents light source spectrum combined with illuminant
 */
struct RGBIlluminantSpectrum {
    float scale;                        ///< Intensity scaling factor
    RGBSigmoidPolynomial rsp;           ///< Spectral shape coefficients
    DenselySampledSpectrum illuminant;  ///< Illuminant spectrum (e.g., D65)
};

/**
 * @brief Creates an illuminant spectrum from RGB color
 * @param cs RGB color space definition
 * @param rgb Input RGB color
 * @return Initialized illuminant spectrum
 */
RGBIlluminantSpectrum RGBIlluminantSpectrumNew(RGB rgb) {
    RGBIlluminantSpectrum s;
    
    // Compute the maximum component of the RGB vector
    float max_comp = max(rgb.r, max(rgb.g, rgb.b));
    s.scale = 2.0f * max_comp;
    
    // Normalize the RGB vector by scale if not zero
    RGB scaled_rgb = s.scale > 0.0f ? RGBDivFloat(rgb, s.scale) : RGBNew(0.0f, 0.0f, 0.0f);
    s.rsp = ToRGBCoeffs(scaled_rgb);
    
    // Initialize illuminant spectrum (D65)
    // Note: In a real implementation, this would be precomputed and stored
    for (int i = 0; i < NCIESamples; i++) {
        float lambda = float(i + LambdaMin);
        s.illuminant.values[i] = SampleD65Illuminant(lambda);
    }
    
    return s;
}

/**
 * @brief Evaluates full spectral power distribution
 * @param s Input illuminant spectrum
 * @param lambda Wavelength in nanometers
 * @return Spectral radiance value
 */
float RGBIlluminantSpectrumEval(RGBIlluminantSpectrum s, float lambda) {
    float illuminant_val = DenselySampledSpectrumEval(s.illuminant, lambda);

    return s.scale * RGBSigmoidPolynomialEval(s.rsp, lambda) * illuminant_val;
}

/**
 * @brief Computes maximum spectral radiance
 * @param s Input illuminant spectrum
 * @return Peak radiance value
 */
float RGBIlluminantSpectrumMaxValue(RGBIlluminantSpectrum s) {
    float rsp_max = RGBSigmoidPolynomialMaxValue(s.rsp);
    float illuminant_max = DenselySampledSpectrumMaxValue(s.illuminant);

    return s.scale * rsp_max * illuminant_max;
}

#endif // _SPECTRUM__GLSL__