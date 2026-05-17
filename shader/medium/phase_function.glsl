#ifndef PHASE_FUNCTION_GLSL
#define PHASE_FUNCTION_GLSL

#include "util/util.glsl"
#include "sample/sampling.glsl"

// Phase type enumeration, consistent with C++ side
const int PhaseType_HenyeyGreenstein = 0; ///< Henyey-Greenstein phase function

/**
 * @struct PhaseEvalInfo
 * @brief Stores the result of phase function evaluation
 */
struct PhaseEvalInfo {
    SampledSpectrum phase;
    float pdf;
};

/**
 * @struct PhaseSampleInfo
 * @brief Stores the result of sampling a direction from phase function
 */
struct PhaseSampleInfo {
    vec3 world_out;
    SampledSpectrum phase;
    float pdf;
};

/**
 * @brief Evaluates the Henyey-Greenstein phase function
 * @param info Intersection information containing medium properties
 * @param world_in Incoming light direction in world space
 * @param world_out Scattered light direction in world space
 * @return PhaseEvalInfo containing phase function value and PDF
 */
PhaseEvalInfo HenyeyGreensteinEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out) {
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
    float cos_theta = dot(normalize(world_in), normalize(world_out));
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
 * @param world_in Incoming light direction in world space
 * @param sample_xy Random sample coordinates in [0,1) range
 * @return PhaseSampleInfo containing sampled direction, phase value and PDF
 */
PhaseSampleInfo HenyeyGreensteinSample(IntersectionInfo info, vec3 world_in, vec2 sample_xy) {
    PhaseSampleInfo result;
    result.world_out = vec3(0.0f);
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    float g = info.medium.g;

    // Handle isotropic scattering (g = 0)
    if (0.0f == g) {
        // Set phase value for all wavelengths
        result.world_out = UniformSphereSample(sample_xy);
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
    vec3 local_l = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
    
    // Calculate phase function value
    float cubic_term = (1.0f + g * g - 2.0f * g * cos_theta);
    float phase_value = 1.0f / (4.0f * PI) * (1.0f - g * g) / sqrt(cubic_term * cubic_term * cubic_term);
    
    // Set phase value for all wavelengths
    result.phase = SampledSpectrumNewFloat(phase_value);
    result.pdf = phase_value;
    result.world_out = ToWorldFromUp(local_l, world_in);
    
    return result;
}

/**
 * @brief Unified phase function evaluation function that dispatches to the appropriate phase function model
 * @param info Intersection data containing medium properties
 * @param world_in View direction in world space (pointing toward light source)
 * @param world_out Scattering direction in world space
 * @return PhaseEvalInfo Structure containing phase function value and PDF
 */
PhaseEvalInfo PhaseEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out) {
    PhaseEvalInfo result;
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on phase function type
    if (PhaseType_HenyeyGreenstein == info.medium.phase_type) {
        return HenyeyGreensteinEvaluate(info, world_in, world_out);
    }
    // Add more phase function types here in the future
    // else if (PhaseType_Rayleigh == info.medium.phase_type) {
    //     return RayleighEvaluate(info, world_in, world_out);
    // }
    // else if (PhaseType_Mie == info.medium.phase_type) {
    //     return MieEvaluate(info, world_in, world_out);
    // }
    
    // Unknown phase function type, return zero contribution
    return result;
}

/**
 * @brief Unified phase function sampling function that dispatches to the appropriate phase function model
 * @param info Intersection data containing medium properties
 * @param world_in View direction in world space (pointing toward light source)
 * @param sample_xy 2D random sample in [0,1) range
 * @return PhaseSampleInfo Structure containing sampled direction, phase value, and PDF
 */
PhaseSampleInfo PhaseSample(IntersectionInfo info, vec3 world_in, vec2 sample_xy) {
    PhaseSampleInfo result;
    result.world_out = vec3(0.0f);
    result.phase = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on phase function type
    if (PhaseType_HenyeyGreenstein == info.medium.phase_type) {
        return HenyeyGreensteinSample(info, world_in, sample_xy);
    }
    // Add more phase function types here in the future
    // else if (PhaseType_Rayleigh == info.medium.phase_type) {
    //     return RayleighSample(info, world_in, sample_xy);
    // }
    // else if (PhaseType_Mie == info.medium.phase_type) {
    //     return MieSample(info, world_in, sample_xy);
    // }
    
    // Unknown phase function type, return zero contribution
    return result;
}

#endif