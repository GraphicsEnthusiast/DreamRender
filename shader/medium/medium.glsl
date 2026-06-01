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
 * @brief Evaluates wavelength PMF for medium scattering
 * @param beta Spectral beta (path contribution)
 * @param albedo Medium albedo spectrum
 * @return WavelengthEvalInfo containing PMF for each wavelength
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
 * @return WavelengthSampleInfo containing sampled channel and PMF
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
        // No scattering case: transmission through the entire medium
        // PDF = Tr(max_distance) - probability of reaching boundary without interaction
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i];
        }
    } 
    else {
        // Scattering case: interaction inside medium
        // PDF = σ_t * Tr(distance) - joint PDF for scattering at distance t
        // This is equivalent to: PDF = (1 - Tr(max_distance)) * [σ_t * Tr(distance) / (1 - Tr(max_distance))]
        // where (1 - Tr(max_distance)) cancels out between numerator and denominator
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
    
    // Sample distance t from exponential distribution: p(t) = σ_t * exp(-σ_t * t)
    result.distance = -log(max(sample_val, 0.0f)) / info.medium.sigma_t.values[channel];
    result.distance = min(MaxFloat, result.distance);
    
    // ====================================================================
    // Key insight: Sampling distance ≥ max_distance is equivalent to random number ≥ 1 - Tr(max_distance)
    // ====================================================================
    // The inverse CDF of exponential distribution: t = -ln(1-u)/σ_t
    // The probability of transmission (distance ≥ max_distance) is: P(distance ≥ max_distance) = 1 - CDF(max_distance) = exp(-σ_t * max_distance) = Tr(max_distance)
    // This corresponds to: u ≥ 1 - Tr(max_distance)  (since 1-u ≤ Tr(max_distance) when t ≥ max_distance)
    //
    // Let's verify:
    // CDF(distance) = 1 - exp(-σ_t * distance)
    // distance = -ln(1-u)/σ_t
    // For distance ≥ max_distance: -ln(1-u)/σ_t ≥ max_distance ⇒ ln(1-u) ≤ -σ_t * max_distance ⇒ 1-u ≤ exp(-σ_t * max_distance) = Tr(max_distance)
    // Therefore: u ≥ 1 - Tr(max_distance) when distance ≥ max_distance
    //
    // In code: we sample distance and compare with max_distance, which is mathematically equivalent to
    // comparing u with 1 - Tr(max_distance), but avoids computing Tr(max_distance) explicitly.
    // ====================================================================
    
    // P (distance ≥ max_distance) = P(u ≥ 1 - Tr(max_distance)) = Tr(max_distance)
    if (result.distance >= max_distance) {
        result.distance = max_distance;
        result.scattered = false;  // Transmission event
        
        // Calculate transmittance for boundary hit: Tr(max_distance)
        SampledSpectrum sigma_t_dist = MulFloat(info.medium.sigma_t, max_distance);
        SampledSpectrum trans = Exp(Negate(sigma_t_dist));  // Tr(max_distance)
        
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i];
        }
        
        result.transmittance = trans;
    }
    // P (distance < max_distance) = 1 - P(u ≥ 1 - Tr(max_distance)) = 1 - Tr(max_distance)
    else {
        result.scattered = true;  // Scattering event
        
        // Calculate transmittance at scattering point: Tr(distance)
        SampledSpectrum sigma_t_dist = MulFloat(info.medium.sigma_t, result.distance);
        SampledSpectrum trans = Exp(Negate(sigma_t_dist));  // Tr(distance)
        
        // ====================================================================
        // Detailed explanation of PDF calculation and (1 - Tr(max_distance)) cancellation
        // ====================================================================
        // 
        // 1. Mathematical Derivation of Conditional PDF:
        // Let p(t) = σ_t * exp(-σ_t * t) = σ_t * Tr(t) be the unconditional PDF
        // The probability of interaction within [0, max_distance) is:
        //   P_interact = ∫[0, max_distance] p(t) dt = 1 - exp(-σ_t * max_distance) = 1 - Tr(max_distance)
        // 
        // The conditional PDF given that interaction occurs within [0, max_distance) is:
        //   p_cond(t) = p(t) / P_interact = [σ_t * Tr(t)] / [1 - Tr(max_distance)]  for 0 ≤ t < max_distance
        // 
        // 2. Why divide by (1 - Tr(max_distance))? Proof by integration:
        //   ∫[0, max_distance] p_cond(t) dt = ∫[0, max_distance] [σ_t * Tr(t)] / [1 - Tr(max_distance)] dt
        //   = (1 / [1 - Tr(max_distance)]) * ∫[0, max_distance] σ_t * Tr(t) dt
        //   = (1 / [1 - Tr(max_distance)]) * [1 - Tr(max_distance)]
        //   = 1
        // This proves p_cond(t) is a valid PDF (integrates to 1) over [0, max_distance).
        // 
        // 3. The joint PDF for scattering at distance t is:
        //   pdf_joint(t) = P_interact * p_cond(t) = [1 - Tr(max_distance)] * [σ_t * Tr(t) / (1 - Tr(max_distance))]
        //   = σ_t * Tr(t)
        // The factor (1 - Tr(max_distance)) cancels in the joint PDF!
        // 
        // 4. In Monte Carlo weight calculation for scattering:
        //   weight = contribution / pdf
        //   = [σ_s * Tr(t) * S(t)] / [σ_t * Tr(t)]
        //   = (σ_s / σ_t) * S(t)
        // Here, both Tr(t) cancels and the implicit (1 - Tr(max_distance)) cancels.
        // ====================================================================
        
        // PDF for scattering event: σ_t * Tr(t)
        // This is equivalent to: (1 - Tr(max_distance)) * [σ_t * Tr(t) / (1 - Tr(max_distance))]
        // The factor (1 - Tr(max_distance)) cancels out in weight calculation
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i] * info.medium.sigma_t.values[i];
        }
        
        // For scattering, multiply by σ_s for the contribution
        result.transmittance = Mul(trans, info.medium.sigma_s);
    }
    
    // Check if transmittance is valid (non-zero)
    bool valid = false;
    for (int i = 0; i < NSpectrumSamples; i++) {
        if (result.transmittance.values[i] > 0.0f) {
            valid = true;
        }
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