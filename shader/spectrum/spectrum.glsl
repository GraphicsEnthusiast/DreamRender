#ifndef _SPECTRUM__GLSL__
#define _SPECTRUM__GLSL__

// Spectral rendering constants
const uint NSpectrumSamples = 8;     // Number of spectral samples per calculation
const float LambdaMin = 360.0f;      // Minimum visible wavelength (nanometers)
const float LambdaMax = 830.0f;      // Maximum visible wavelength (nanometers)

/**
 * @struct Spectrum
 * @brief Represents spectral distribution with discrete wavelength samples
 */
struct Spectrum {
    float val[NSpectrumSamples];  // Spectral values at sampled wavelengths
};

/**
 * @brief Creates a constant spectrum
 * @param c Constant value for all wavelengths
 * @return New Spectrum
 */
Spectrum SpectrumNew(float c[NSpectrumSamples]) {
    Spectrum s;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        s.val[i] = c[i];
    }

    return s;
}

/**
 * @brief Adds two spectra component-wise
 * @param a First spectrum operand
 * @param b Second spectrum operand
 * @return Component-wise sum of a and b
 */
Spectrum SpectrumAdd(Spectrum a, Spectrum b) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = a.val[i] + b.val[i];
    }

    return r;
}

/**
 * @brief Subtracts two spectra component-wise
 * @param a First spectrum operand (minuend)
 * @param b Second spectrum operand (subtrahend)
 * @return Component-wise difference of a and b (a - b)
 */
Spectrum SpectrumSub(Spectrum a, Spectrum b) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = a.val[i] - b.val[i];
    }

    return r;
}

/**
 * @brief Multiplies two spectra component-wise (Hadamard product)
 * @param a First spectrum operand
 * @param b Second spectrum operand
 * @return Component-wise product of a and b
 */
Spectrum SpectrumMul(Spectrum a, Spectrum b) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = a.val[i] * b.val[i];
    }

    return r;
}

/**
 * @brief Scales a spectrum by a scalar factor
 * @param s Spectrum to scale
 * @param a Scalar multiplier
 * @return Scaled spectrum
 */
Spectrum SpectrumMulFloat(Spectrum s, float a) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = s.val[i] * a;
    }

    return r;
}

/**
 * @brief Divides two spectra component-wise with zero-division protection
 * @param a Numerator spectrum
 * @param b Denominator spectrum
 * @return Component-wise quotient a/b (returns 0 where denominator is 0)
 */
Spectrum SpectrumDiv(Spectrum a, Spectrum b) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = (b.val[i] != 0.0f) ? (a.val[i] / b.val[i]) : 0.0f;
    }

    return r;
}

/**
 * @brief Divides a spectrum by a scalar divisor
 * @param s Spectrum to divide
 * @param a Scalar divisor
 * @return Component-wise quotient s/a
 */
Spectrum SpectrumDivFloat(Spectrum s, float a) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = s.val[i] / a;
    }

    return r;
}

/**
 * @brief Negates all components of a spectrum
 * @param s Input spectrum
 * @return Negated spectrum
 */
Spectrum SpectrumNegate(Spectrum s) {
    Spectrum r;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        r.val[i] = -s.val[i];
    }

    return r;
}

/**
 * @brief Checks if any spectral component is non-zero
 * @param s Spectrum to check
 * @return True if at least one component is non-zero, false otherwise
 */
bool SpectrumNonZero(Spectrum s) {
    for (uint i = 0; i < NSpectrumSamples; i++) {
        if (0.0f != s.val[i]) {
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
float SpectrumMin(Spectrum s) {
    float m = s.val[0];
    for (uint i = 1; i < NSpectrumSamples; i++) {
        m = min(m, s.val[i]);
    }

    return m;
}

/**
 * @brief Finds the maximum value across all spectral components
 * @param s Input spectrum
 * @return Maximum component value
 */
float SpectrumMax(Spectrum s) {
    float m = s.val[0];
    for (uint i = 1; i < NSpectrumSamples; i++) {
        m = max(m, s.val[i]);
    }

    return m;
}

/**
 * @brief Computes the arithmetic mean of spectral components
 * @param s Input spectrum
 * @return Average value of all spectral samples
 */
float SpectrumAvg(Spectrum s) {
    float sum = s.val[0];
    for (uint i = 1; i < NSpectrumSamples; i++) {
        sum += s.val[i];
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
    for (uint i = 0; i < NSpectrumSamples; i++) {
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
    for (uint i = 1; i < NSpectrumSamples; i++) {
        swl.lambda[i] = swl.lambda[i - 1] + delta;
        if (swl.lambda[i] > lambda_max) {
            swl.lambda[i] = lambda_min + (swl.lambda[i] - lambda_max);
        }
    }
    
    // Compute uniform PDF
    float pdf_val = 1.0f / (lambda_max - lambda_min);
    for (uint i = 0; i < NSpectrumSamples; i++) {
        swl.pdf[i] = pdf_val;
    }
    
    return swl;
}

/**
 * @brief Terminates secondary wavelengths (keeps only primary)
 * @param swl Inout reference to SampledWavelengths instance
 */
void SampledWavelengthsTerminateSecondary(inout SampledWavelengths swl) {
    bool terminated = true;
    for (uint i = 1; i < NSpectrumSamples; i++) {
        if (swl.pdf[i] != 0.0f) {
            terminated = false;
            break;
        }
    }
    if (terminated) return;
    
    // Update probabilities
    for (uint i = 1; i < NSpectrumSamples; i++) {
        swl.pdf[i] = 0.0f;
    }
    swl.pdf[0] /= float(NSpectrumSamples);
}

/**
 * @brief Checks if secondary wavelengths have been terminated
 * @param swl SampledWavelengths instance
 */
bool SampledWavelengthsSecondaryTerminated(SampledWavelengths swl) {
    for (uint i = 1; i < NSpectrumSamples; i++) {
        if (swl.pdf[i] != 0.0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Gets PDF values as Spectrum
 * @param swl SampledWavelengths instance
 */
Spectrum SampledWavelengthsPDF(SampledWavelengths swl) {
    Spectrum s;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        s.val[i] = swl.pdf[i];
    }
    return s;
}

const uint DenselySpectrumSize = uint(LambdaMax - LambdaMin) + 1;  // Number of samples (471)

/**
 * @struct DenselySampledSpectrum
 * @brief Stores densely sampled spectral data (1nm intervals)
 */
struct DenselySampledSpectrum {
    float values[DenselySpectrumSize];  // Spectral value array
};

/**
 * @brief Creates a zero-initialized densely sampled spectrum
 * @return Initialized spectrum struct
 */
DenselySampledSpectrum DenselySampledSpectrumCreate() {
    DenselySampledSpectrum s;
    for (uint i = 0; i < DenselySpectrumSize; i++) {
        s.values[i] = 0.0f;
    }
    return s;
}

/**
 * @brief Samples spectral values at discrete wavelengths
 * @param d Input densely sampled spectrum
 * @param lambda Target wavelength samples
 * @return Spectrum values at sampled wavelengths
 */
Spectrum DenselySampledSpectrumSample(DenselySampledSpectrum d, SampledWavelengths lambda) {
    Spectrum s;
    for (uint i = 0; i < NSpectrumSamples; i++) {
        // Calculate array index (rounded to nearest integer)
        int idx = int(round(lambda.lambda[i]) - int(LambdaMin));
        
        if (idx < 0 || idx >= DenselySpectrumSize) {
            s.val[i] = 0.0f;  // Return 0 for out-of-range wavelengths
        } else {
            s.val[i] = d.values[idx];
        }
    }
    return s;
}

/**
 * @brief Scales spectral values by a factor
 * @param d Input/output spectrum (modified in-place)
 * @param scale Scaling factor
 */
void DenselySampledSpectrumScale(inout DenselySampledSpectrum d, float scale) {
    for (uint i = 0; i < DenselySpectrumSize; i++) {
        d.values[i] *= scale;
    }
}

/**
 * @brief Finds maximum spectral value
 * @param d Input spectrum
 * @return Maximum spectral value
 */
float DenselySampledSpectrumMaxValue(DenselySampledSpectrum d) {
    float maxVal = d.values[0];
    for (uint i = 1; i < DenselySpectrumSize; i++) {
        maxVal = max(maxVal, d.values[i]);
    }

    return maxVal;
}

/**
 * @brief Evaluates spectrum at specified wavelength
 * @param d Input spectrum
 * @param lambda Query wavelength
 * @return Spectral value (0 if wavelength out-of-range)
 */
float DenselySampledSpectrumEval(DenselySampledSpectrum d, float lambda) {
    int idx = int(round(lambda) - int(LambdaMin));
    if (idx < 0 || idx >= DenselySpectrumSize) {
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
    for (uint i = 0; i < DenselySpectrumSize; i++) {
        if (a.values[i] != b.values[i]) {
            return false;
        }
    }

    return true;
}

#endif