#ifndef MEDIUM_GLSL
#define MEDIUM_GLSL

#include "medium/phase_function.glsl"

// Medium type enumeration, consistent with C++ side
const int MediumType_Homogeneous = 0; ///< Homogeneous medium

/**
 * @struct WavelengthEvalInfo
 * @brief Info of wavelength PMF evaluation
 */
struct WavelengthEvalInfo {
    SampledSpectrum pmf;
};

/**
 * @struct WavelengthSampleInfo
 * @brief Info of wavelength sampling
 */
struct WavelengthSampleInfo {
    int channel;
    SampledSpectrum pmf;
};

/**
 * @struct MediumEvalInfo
 * @brief Info of medium distance evaluation
 */
struct MediumEvalInfo {
    SampledSpectrum transmittance;
    float pdf;
};

/**
 * @struct MediumSampleInfo
 * @brief Info of medium distance sampling
 */
struct MediumSampleInfo {
    SampledSpectrum transmittance;
    float distance;
    float pdf;
    bool scattered;
};

/**
 * @brief Evaluates wavelength PDF for medium scattering
 * @param beta Spectral beta (path contribution)
 * @param albedo Medium albedo spectrum
 * @return WavelengthEvalInfo containing PDF for each wavelength
 */
WavelengthEvalInfo MediumWavelengthEvaluate(SampledSpectrum beta, SampledSpectrum albedo) {
    WavelengthEvalInfo result;
    result.pmf = SampledSpectrumNewFloat(0.0f);
    
    // Create empirical discrete distribution: beta * albedo
    SampledSpectrum history_albedo = Mul(beta, albedo);
    
    // Create binary table from the combined spectrum
    BinaryTable1D wave_table = BinaryTableNew(history_albedo);
    
    // Extract PMF from the binary table
    for (int i = 0; i < NSpectrumSamples; i++) {
        result.pmf.values[i] = wave_table.pmf[i];
    }
    
    return result;
}

/**
 * @brief Samples wavelength for medium scattering
 * @param beta Spectral beta (path contribution)
 * @param albedo Medium albedo spectrum
 * @param u Random value in [0, 1)
 * @return WavelengthSampleInfo containing sampled channel and PDF
 */
WavelengthSampleInfo MediumWavelengthSample(SampledSpectrum beta, SampledSpectrum albedo, float u) {
    WavelengthSampleInfo result;
    result.channel = 0;
    result.pmf = SampledSpectrumNewFloat(0.0f);
    
    // Create empirical discrete distribution: beta * albedo
    SampledSpectrum history_albedo = Mul(beta, albedo);
    
    // Create binary table from the combined spectrum
    BinaryTable1D wave_table = BinaryTableNew(history_albedo);
    
    // Extract PMF from the binary table
    for (int i = 0; i < NSpectrumSamples; i++) {
        result.pmf.values[i] = wave_table.pmf[i];
    }
    
    // Sample index of wavelength from empirical discrete distribution
    result.channel = BinaryTableSample(wave_table, u);
    
    return result;
}

/**
 * @brief Evaluates transmittance for a given distance in homogeneous medium
 * @param info Intersection info containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo HomogeneousDistanceEvaluate(IntersectionInfo info, SampledSpectrum beta, bool scattered) {
    MediumEvalInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    float distance = info.distance;
    // Evaluate wavelength PMF
    SampledSpectrum albedo = Div(info.medium.sigma_s, info.medium.sigma_t);
    WavelengthEvalInfo wavelength_result = MediumWavelengthEvaluate(beta, albedo);
    SampledSpectrum wavelength_pmf = wavelength_result.pmf;
    
    // Calculate transmittance
    SampledSpectrum sigma_t_dist = MulFloat(info.medium.sigma_t, distance);
    SampledSpectrum trans = Exp(Negate(sigma_t_dist));
    
    // Calculate PDF based on scattering status
    if (!scattered) {
        // No scattering case
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i];
        }
    } 
    else {
        // Scattering case
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i] * info.medium.sigma_t.values[i];
        }
    }
    
    result.transmittance = trans;
    
    // Check if transmittance is valid (non-zero)
    bool valid = false;
    for (int i = 0; i < NSpectrumSamples; i++) {
        if (trans.values[i] > 0.0f) {
            valid = true;
        }
    }
    
    // Apply scattering coefficient if scattered
    if (scattered) {
        result.transmittance = Mul(result.transmittance, info.medium.sigma_s);
    }
    
    // If invalid, set to zero
    if (!valid) {
        result.transmittance = SampledSpectrumNewFloat(0.0f);
    }
    
    return result;
}

/**
 * @brief Samples a distance in homogeneous medium
 * @param info Intersection info containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param sample_xy Random value in [0, 1)
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo HomogeneousDistanceSample(IntersectionInfo info, SampledSpectrum beta, vec2 sample_xy) {
    MediumSampleInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.scattered = false;
    
    float max_distance = info.distance;
    
    // Sample wavelength channel
    SampledSpectrum albedo = Div(info.medium.sigma_s, info.medium.sigma_t);
    WavelengthSampleInfo wavelength_result = MediumWavelengthSample(beta, albedo, sample_xy.x);
    int channel = wavelength_result.channel;
    SampledSpectrum wavelength_pmf = wavelength_result.pmf;
    
    // Sample collision-free distance using exponential distribution
    float u = sample_xy.y;
    float sample_val = 1.0f - u;
    if (sample_val <= 0.0f) {
        sample_val = 0.0f;
    }
    
    result.distance = -log(max(sample_val, 0.0f)) / info.medium.sigma_t.values[channel];
    result.distance = min(MaxFloat, result.distance);
    
    // Check if we hit volume boundary
    if (result.distance >= max_distance) {
        result.distance = max_distance;
        result.scattered = false;
        
        // Calculate transmittance for boundary hit
        SampledSpectrum sigma_t_dist = MulFloat(info.medium.sigma_t, max_distance);
        SampledSpectrum trans = Exp(Negate(sigma_t_dist));
        
        // Calculate PDF for boundary hit
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i];
        }
        
        result.transmittance = trans;
    } 
    else {
        result.scattered = true;
        
        // Calculate transmittance for scattering event
        SampledSpectrum sigma_t_dist = MulFloat(info.medium.sigma_t, result.distance);
        SampledSpectrum trans = Exp(Negate(sigma_t_dist));
        
        // Calculate PDF for scattering event
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i] * info.medium.sigma_t.values[i];
        }
        
        result.transmittance = trans;
    }
    
    // Check if transmittance is valid (non-zero)
    bool valid = false;
    for (int i = 0; i < NSpectrumSamples; i++) {
        if (result.transmittance.values[i] > 0.0f) {
            valid = true;
        }
    }
    
    // Apply scattering coefficient if scattered
    if (result.scattered) {
        result.transmittance = Mul(result.transmittance, info.medium.sigma_s);
    }
    
    // If invalid, set to zero
    if (!valid) {
        result.transmittance = SampledSpectrumNewFloat(0.0f);
    }
    
    return result;
}

/**
 * @brief Unified medium distance evaluation function that dispatches to the appropriate medium model
 * @param info Intersection data containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo MediumDistanceEvaluate(IntersectionInfo info, SampledSpectrum beta, bool scattered) {
    MediumEvalInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on medium type
    if (MediumType_Homogeneous == info.medium.type) {
        return HomogeneousDistanceEvaluate(info, beta, scattered);
    }
    
    // Unknown medium type, return zero contribution
    return result;
}

/**
 * @brief Unified medium distance sampling function that dispatches to the appropriate medium model
 * @param info Intersection data containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param sample_xy Random value in [0, 1)
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo MediumDistanceSample(IntersectionInfo info, SampledSpectrum beta, vec2 sample_xy) {
    MediumSampleInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.scattered = false;
    
    // Dispatch based on medium type
    if (MediumType_Homogeneous == info.medium.type) {
        return HomogeneousDistanceSample(info, beta, sample_xy);
    }
    
    // Unknown medium type, return zero contribution
    return result;
}

#endif // MEDIUM_GLSL