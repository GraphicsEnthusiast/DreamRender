#ifndef MEDIUM_GLSL
#define MEDIUM_GLSL

#include "util/util.glsl"
#include "sample/sampling.glsl"

// Phase type enumeration, consistent with C++ side
const int PhaseType_HenyeyGreenstein = 0; ///< Henyey-Greenstein phase function

// Medium type enumeration, consistent with C++ side
const int MediumType_Homogeneous = 0; ///< Homogeneous medium

/**
 * @struct PhaseEvalInfo
 * @brief Stores the result of phase function evaluation
 */
struct PhaseEvalInfo {
    SampledSpectrum phase;    ///< Phase function value for each channel
    float pdf;                ///< Probability density function value
};

/**
 * @struct PhaseSampleInfo
 * @brief Stores the result of sampling a direction from phase function
 */
struct PhaseSampleInfo {
    vec3 world_l;             ///< Sampled direction vector
    SampledSpectrum phase;    ///< Phase function value for each channel
    float pdf;                ///< Probability density function value
};

/**
 * @struct WavelengthEvalInfo
 * @brief Info of wavelength PDF evaluation
 */
struct WavelengthEvalInfo {
    SampledSpectrum pdf;      ///< Probability density function for each wavelength
};

/**
 * @struct WavelengthSampleInfo
 * @brief Info of wavelength sampling
 */
struct WavelengthSampleInfo {
    int channel;              ///< Sampled wavelength channel index
    SampledSpectrum pdf;      ///< Probability density function for each wavelength
};

/**
 * @struct MediumEvalInfo
 * @brief Info of medium distance evaluation
 */
struct MediumEvalInfo {
    SampledSpectrum transmittance;  ///< Transmittance spectrum
    float pdf;                ///< Transmittance PDF
};

/**
 * @struct MediumSampleInfo
 * @brief Info of medium distance sampling
 */
struct MediumSampleInfo {
    SampledSpectrum transmittance;  ///< Transmittance spectrum
    float distance;                 ///< Sampled distance
    float pdf;                ///< Transmittance PDF
    bool scattered;                 ///< Whether scattering occurred
};

/**
 * @brief Evaluates the Henyey-Greenstein phase function
 * @param info Intersection information containing medium properties
 * @param world_v Incoming light direction in world space
 * @param world_l Scattered light direction in world space
 * @return PhaseEvalInfo containing phase function value and PDF
 */
PhaseEvalInfo HenyeyGreensteinEvaluate(IntersectionInfo info, vec3 world_v, vec3 world_l) {
    PhaseEvalInfo result;
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    float g = info.medium.g;
    // Handle isotropic scattering (g = 0)
    if (0.0f == g) {
        result.pdf = UniformSpherePDF();
        // Set phase value for all wavelengths
        result.phase = SampledSpectrumNewFloat(1.0f / (4.0f * PI));

        return result;
    }
    
    // Henyey-Greenstein evaluation
    float cos_theta = dot(normalize(world_v), normalize(world_l));
    float cubic_term = (1.0f + g * g - 2.0f * g * cos_theta);
    float phase_value = 1.0f / (4.0f * PI) * (1.0f - g * g) / sqrt(cubic_term * cubic_term * cubic_term);
    
    // Set phase value for all wavelengths
    result.phase = SampledSpectrumNewFloat(phase_value);
    result.pdf = phase_value;
    
    return result;
}

/**
 * @brief Samples a scattering direction according to the Henyey-Greenstein phase function
 * @param info Intersection information containing medium properties
 * @param world_v Incoming light direction in world space
 * @param sample_xy Random sample coordinates in [0,1) range
 * @return PhaseSampleInfo containing sampled direction, phase value and PDF
 */
PhaseSampleInfo HenyeyGreensteinSample(IntersectionInfo info, vec3 world_v, vec2 sample_xy) {
    PhaseSampleInfo result;
    result.world_l = vec3(0.0f);
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    float g = info.medium.g;
    vec3 local_l;
    // Handle isotropic scattering (g = 0)
    if (0.0f == g) {
        local_l = UniformSphereSample(sample_xy);
        
        // Set phase value for all wavelengths
        result.world_l = ToWorldFromUp(local_l, world_v);
        result.phase = SampledSpectrumNewFloat(1.0f / (4.0f * PI));
        result.pdf = UniformSpherePDF();

        return result;
    }

    float u1 = sample_xy.x;
    float u2 = sample_xy.y;
    
    // Henyey-Greenstein sampling
    float cos_theta;
    if (abs(g) < Epsilon) {
        cos_theta = 1.0f - 2.0f * u1;
    } 
    else {
        float sqrt_term = (1.0f - g * g) / (1.0f - g + 2.0f * g * u1);
        cos_theta = (1.0f + g * g - sqrt_term * sqrt_term) / (2.0f * g);
    }
    
    // Clamp to valid range
    cos_theta = clamp(cos_theta, -1.0f, 1.0f);
    float sin_theta = sqrt(max(0.0f, 1.0f - cos_theta * cos_theta));
    float phi = 2.0f * PI * u2;
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);
    
    // Create direction in local coordinate system (z-up)
    local_l = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
    
    // Calculate phase function value
    float cubic_term = (1.0f + g * g - 2.0f * g * cos_theta);
    float phase_value = 1.0f / (4.0f * PI) * (1.0f - g * g) / sqrt(cubic_term * cubic_term * cubic_term);
    
    // Set phase value for all wavelengths
    result.phase = SampledSpectrumNewFloat(phase_value);
    result.pdf = phase_value;
    result.world_l = ToWorldFromUp(local_l, world_v);
    
    return result;
}

/**
 * @brief Unified phase function evaluation function that dispatches to the appropriate phase function model
 * @param info Intersection data containing medium properties
 * @param world_v View direction in world space (pointing toward light source)
 * @param world_l Scattering direction in world space
 * @return PhaseEvalInfo Structure containing phase function value and PDF
 */
PhaseEvalInfo PhaseEvaluate(IntersectionInfo info, vec3 world_v, vec3 world_l) {
    PhaseEvalInfo result;
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on phase function type
    if (PhaseType_HenyeyGreenstein == info.medium.phase_type) {
        return HenyeyGreensteinEvaluate(info, world_v, world_l);
    }
    // Add more phase function types here in the future
    // else if (PhaseType_Rayleigh == info.medium.phase_type) {
    //     return RayleighEvaluate(info, world_v, world_l);
    // }
    // else if (PhaseType_Mie == info.medium.phase_type) {
    //     return MieEvaluate(info, world_v, world_l);
    // }
    
    // Unknown phase function type, return zero contribution
    return result;
}

/**
 * @brief Unified phase function sampling function that dispatches to the appropriate phase function model
 * @param info Intersection data containing medium properties
 * @param world_v View direction in world space (pointing toward light source)
 * @param sample_xy 2D random sample in [0,1) range
 * @return PhaseSampleInfo Structure containing sampled direction, phase value, and PDF
 */
PhaseSampleInfo PhaseSample(IntersectionInfo info, vec3 world_v, vec2 sample_xy) {
    PhaseSampleInfo result;
    result.world_l = vec3(0.0f);
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on phase function type
    if (PhaseType_HenyeyGreenstein == info.medium.phase_type) {
        return HenyeyGreensteinSample(info, world_v, sample_xy);
    }
    // Add more phase function types here in the future
    // else if (PhaseType_Rayleigh == info.medium.phase_type) {
    //     return RayleighSample(info, world_v, sample_xy);
    // }
    // else if (PhaseType_Mie == info.medium.phase_type) {
    //     return MieSample(info, world_v, sample_xy);
    // }
    
    // Unknown phase function type, return zero contribution
    return result;
}

/**
 * @brief Evaluates wavelength PDF for medium scattering
 * @param beta Spectral beta (path contribution)
 * @param albedo Medium albedo spectrum
 * @return WavelengthEvalInfo containing PDF for each wavelength
 */
WavelengthEvalInfo MediumWavelengthEvaluate(SampledSpectrum beta, SampledSpectrum albedo) {
    WavelengthEvalInfo result;
    result.pdf = SampledSpectrumNewFloat(0.0f);
    
    // Create empirical discrete distribution: beta * albedo
    SampledSpectrum history_albedo = Mul(beta, albedo);
    
    // Create binary table from the combined spectrum
    BinaryTable1D wave_table = BinaryTableNew(history_albedo);
    
    // Extract PDF from the binary table
    for (int i = 0; i < NSpectrumSamples; i++) {
        result.pdf.values[i] = wave_table.pdf[i];
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
    result.pdf = SampledSpectrumNewFloat(0.0f);
    
    // Create empirical discrete distribution: beta * albedo
    SampledSpectrum history_albedo = Mul(beta, albedo);
    
    // Create binary table from the combined spectrum
    BinaryTable1D wave_table = BinaryTableNew(history_albedo);
    
    // Extract PDF from the binary table
    for (int i = 0; i < NSpectrumSamples; i++) {
        result.pdf.values[i] = wave_table.pdf[i];
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
    // Evaluate wavelength PDF
    SampledSpectrum albedo = Div(info.medium.sigma_s, info.medium.sigma_t);
    WavelengthEvalInfo wavelength_result = MediumWavelengthEvaluate(beta, albedo);
    SampledSpectrum wavelength_pdf = wavelength_result.pdf;
    
    // Calculate transmittance
    SampledSpectrum sigma_t_dist = MulFloat(info.medium.sigma_t, distance);
    SampledSpectrum trans = Exp(Negate(sigma_t_dist));
    
    // Calculate PDF based on scattering status
    if (!scattered) {
        // No scattering case
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pdf.values[i] * trans.values[i];
        }
    } 
    else {
        // Scattering case
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pdf.values[i] * trans.values[i] * info.medium.sigma_t.values[i];
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
    SampledSpectrum wavelength_pdf = wavelength_result.pdf;
    
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
            result.pdf += wavelength_pdf.values[i] * trans.values[i];
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
            result.pdf += wavelength_pdf.values[i] * trans.values[i] * info.medium.sigma_t.values[i];
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