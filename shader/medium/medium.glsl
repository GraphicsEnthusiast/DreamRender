#ifndef MEDIUM_GLSL
#define MEDIUM_GLSL

#include "medium/phase_function.glsl"

uniform samplerBuffer Density;

// Medium type enumeration, consistent with C++ side
const int MediumType_Homogeneous = 0;   ///< Homogeneous medium
const int MediumType_Heterogeneous = 1; ///< Heterogeneous medium

/**
 * @brief Samples density from the unified density TBO
 * @param density_offset ID (offset) of the density field in the TBO
 * @param grid_coord Grid coordinates (x, y, z) in the density grid
 * @param grid_size Grid dimensions
 * @return Density value at the given grid position, 0 if out of bounds
 */
float SampleDensity(float density_offset, ivec3 grid_coord, ivec3 grid_size) {
    if (density_offset < 0.0f) {
        return 0.0f;
    }
    
    // Check bounds, return 0 if out of bounds (matching CUDA d() function)
    if (grid_coord.x < 0 || grid_coord.x > grid_size.x - 1 ||
        grid_coord.y < 0 || grid_coord.y > grid_size.y - 1 ||
        grid_coord.z < 0 || grid_coord.z > grid_size.z - 1) {
        return 0.0f;
    }
    
    // Calculate linear index within the density field
    int linear_index = grid_coord.z * grid_size.x * grid_size.y + 
                       grid_coord.y * grid_size.x + 
                       grid_coord.x;
    
    // Sample from the unified density TBO using the density field ID as offset
    return texelFetch(Density, int(density_offset) + linear_index).r;
}

/**
 * @brief Samples density at a normalized position using trilinear interpolation
 * @param density_offset ID (offset) of the density field in the TBO
 * @param p Normalized position in [0, 1]^3 within the density grid
 * @param grid_size Density grid dimensions
 * @return Density value (raw, unnormalized)
 */
float SampleDensityAtPosition(float density_offset, vec3 p, vec3 grid_size) {
    if (density_offset < 0.0f) {
        return 0.0f;
    }
    
    // Convert normalized position to grid coordinates (matching CUDA: p * grid_size)
    vec3 ps = p * grid_size;
    vec3 psi = floor(ps);
    vec3 delta = ps - psi;
    
    // Convert to integer grid coordinates
    ivec3 i0 = ivec3(psi);
    
    // Trilinear interpolation (matching CUDA getDensity function)
    float d00 = mix(SampleDensity(density_offset, i0 + ivec3(0, 0, 0), ivec3(grid_size)),
                    SampleDensity(density_offset, i0 + ivec3(1, 0, 0), ivec3(grid_size)), delta.x);
    float d10 = mix(SampleDensity(density_offset, i0 + ivec3(0, 1, 0), ivec3(grid_size)),
                    SampleDensity(density_offset, i0 + ivec3(1, 1, 0), ivec3(grid_size)), delta.x);
    float d01 = mix(SampleDensity(density_offset, i0 + ivec3(0, 0, 1), ivec3(grid_size)),
                    SampleDensity(density_offset, i0 + ivec3(1, 0, 1), ivec3(grid_size)), delta.x);
    float d11 = mix(SampleDensity(density_offset, i0 + ivec3(0, 1, 1), ivec3(grid_size)),
                    SampleDensity(density_offset, i0 + ivec3(1, 1, 1), ivec3(grid_size)), delta.x);
    
    float d0 = mix(d00, d10, delta.y);
    float d1 = mix(d01, d11, delta.y);
    
    return mix(d0, d1, delta.z);
}

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
 * @param sobol_sampler Sobol sequence sampler (consumes 2 samples)
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo HomogeneousDistanceSample(IntersectionInfo info, SampledSpectrum beta, inout SobolSampler sobol_sampler) {
    MediumSampleInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.scattered = false;
    
    float max_distance = info.distance;
    
    // Sample wavelength channel
    SampledSpectrum albedo = Div(info.medium.sigma_s, info.medium.sigma_t);
    float wavelength_sample = SobolSamplerGet1(sobol_sampler);
    WavelengthSampleInfo wavelength_result = MediumWavelengthSample(beta, albedo, wavelength_sample);
    int channel = wavelength_result.channel;
    SampledSpectrum wavelength_pmf = wavelength_result.pmf;
    
    // Sample collision-free distance using exponential distribution
    float u = SobolSamplerGet1(sobol_sampler);
    float sample_val = 1.0f - u;
    if (sample_val <= 0.0f) {
        sample_val = 0.0f;
    }
    
    // Sample distance t from exponential distribution: p(t) = σ_t * exp(-σ_t * t)
    result.distance = -log(max(sample_val, 0.0f)) / info.medium.sigma_t.values[channel];
    result.distance = min(MaxFloat, result.distance);
    
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
        
        // PDF for scattering event: σ_t * Tr(t)
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
 * @brief Samples a distance in heterogeneous medium using Delta Tracking
 * @param info Intersection info containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param ray_direction Ray direction at the intersection point (normalized)
 * @param sobol_sampler Sobol sequence sampler (consumes random samples)
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo HeterogeneousDistanceSample(IntersectionInfo info, SampledSpectrum beta, vec3 ray_direction, inout SobolSampler sobol_sampler) {
    MediumSampleInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.scattered = false;
    
    float max_distance = info.distance;
    vec3 ray_origin = info.position;
    
    // Compute wavelength-independent majorant
    SampledSpectrum sigma_t_max = MulFloat(info.medium.sigma_t, info.medium.max_density);
    float majorant = Max(sigma_t_max);
    float inv_majorant = 1.0f / majorant;
    SampledSpectrum majorant_spectrum = SampledSpectrumNewFloat(majorant);
    
    // Sample wavelength channel
    SampledSpectrum sigma_a_base = Sub(info.medium.sigma_t, info.medium.sigma_s);
    SampledSpectrum majorant_minus_sigma_a = Sub(majorant_spectrum, sigma_a_base);
    SampledSpectrum wavelength_albedo = MulFloat(majorant_minus_sigma_a, inv_majorant);
    float wavelength_sample = SobolSamplerGet1(sobol_sampler);
    WavelengthSampleInfo wavelength_result = MediumWavelengthSample(beta, wavelength_albedo, wavelength_sample);
    int channel = wavelength_result.channel;
    SampledSpectrum wavelength_pmf = wavelength_result.pmf;
    
    // Delta tracking
    float t = 0.0f;
    SampledSpectrum tr = SampledSpectrumNewFloat(1.0f);  // Accumulated transmittance
    int iter = 64;
    vec3 d = info.medium.max_corner - info.medium.min_corner;
    
    while (true) {
        // Sample collision-free distance with majorant
        float u = SobolSamplerGet1(sobol_sampler);
        float s = -log(max(1.0f - u, 0.0f)) * inv_majorant;
        t += s;
        
        // Reached boundary: transmission event
        if (t > max_distance - Epsilon) {
            result.distance = max_distance;
            result.scattered = false;
            
            // PDF = pmf * Tr (accumulated transmittance)
            for (int i = 0; i < NSpectrumSamples; i++) {
                result.pdf += wavelength_pmf.values[i] * tr.values[i];
            }
            
            result.transmittance = tr;
            return result;
        }
        
        // Compute density and sigma values at current position
        vec3 p = ray_origin + ray_direction * t;
        vec3 normalized_p = (p - info.medium.min_corner) / d;
        float density = SampleDensityAtPosition(info.medium.density_offset, normalized_p, info.medium.grid_size);
        
        SampledSpectrum sigma_s = MulFloat(info.medium.sigma_s, density);
        SampledSpectrum sigma_a = MulFloat(Sub(info.medium.sigma_t, info.medium.sigma_s), density);
        SampledSpectrum sigma_n = Sub(Sub(majorant_spectrum, sigma_a), sigma_s);
        
        // Three-event probabilities (normalized by majorant)
        SampledSpectrum P_s = MulFloat(sigma_s, inv_majorant);
        SampledSpectrum P_a = MulFloat(sigma_a, inv_majorant);
        SampledSpectrum P_n = MulFloat(sigma_n, inv_majorant);
        
        // Event decision using sampled channel
        float rr_sample = SobolSamplerGet1(sobol_sampler);
        float P_s_channel = P_s.values[channel];
        float P_a_channel = P_a.values[channel];
        
        // In-scattering event
        if (rr_sample < P_s_channel) {
            result.distance = t;
            result.scattered = true;
            
            // PDF = pmf * majorant * Tr * P_s (per channel)
            for (int i = 0; i < NSpectrumSamples; i++) {
                result.pdf += wavelength_pmf.values[i] * majorant * tr.values[i] * P_s.values[i];
            }
            
            // Scattering event returns Tr * sigma_s
            result.transmittance = Mul(tr, sigma_s);
            return result;
        }
        // Absorption event
        else if (rr_sample < P_s_channel + P_a_channel) {
            result.distance = t;
            result.scattered = false;
            
            // Absorption terminates path, PDF = pmf * majorant * Tr * P_a
            for (int i = 0; i < NSpectrumSamples; i++) {
                result.pdf += wavelength_pmf.values[i] * majorant * tr.values[i] * P_a.values[i];
            }
            
            result.transmittance = SampledSpectrumNewFloat(0.0f);  // No transmittance after absorption
            return result;
        }
        // Null-scattering event
        else {
            // Update transmittance: tr *= P_n (per channel)
            for (int i = 0; i < NSpectrumSamples; i++) {
                tr.values[i] *= P_n.values[i];
            }
        }
        
        if (--iter == 0) {
            break;
        }
    }
    
    // Iteration limit reached: conservatively return transmission
    result.distance = max_distance;
    result.scattered = false;
    result.transmittance = tr;
    for (int i = 0; i < NSpectrumSamples; i++) {
        result.pdf += wavelength_pmf.values[i] * tr.values[i];
    }
    
    return result;
}

/**
 * @brief Evaluates transmittance for a given distance in heterogeneous medium using Delta Tracking
 * @param info Intersection info containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @param ray_direction Ray direction at the intersection point (normalized)
 * @param sobol_sampler Sobol sequence sampler (consumes random samples)
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo HeterogeneousDistanceEvaluate(IntersectionInfo info, SampledSpectrum beta, bool scattered, vec3 ray_direction, inout SobolSampler sobol_sampler) {
    MediumEvalInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    float max_distance = info.distance;
    vec3 ray_origin = info.position;
    
    // Compute majorant
    SampledSpectrum sigma_t_max = MulFloat(info.medium.sigma_t, info.medium.max_density);
    float majorant = Max(sigma_t_max);
    float inv_majorant = 1.0f / majorant;
    SampledSpectrum majorant_spectrum = SampledSpectrumNewFloat(majorant);
    
    // Per-channel transmittance
    SampledSpectrum tr = SampledSpectrumNewFloat(1.0f);
    
    float t = 0.0f;
    int iter = 64;
    vec3 d = info.medium.max_corner - info.medium.min_corner;
    
    while (true) {
        // Sample collision-free distance
        float u = SobolSamplerGet1(sobol_sampler);
        float s = -log(max(1.0f - u, 0.0f)) * inv_majorant;
        t += s;
        
        // Reached boundary
        if (t > max_distance - Epsilon) {
            break;
        }
        
        // Compute density and sigma values
        vec3 p = ray_origin + ray_direction * t;
        vec3 normalized_p = (p - info.medium.min_corner) / d;
        float density = SampleDensityAtPosition(info.medium.density_offset, normalized_p, info.medium.grid_size);
        
        SampledSpectrum sigma_s = MulFloat(info.medium.sigma_s, density);
        SampledSpectrum sigma_a = MulFloat(Sub(info.medium.sigma_t, info.medium.sigma_s), density);
        SampledSpectrum sigma_n = Sub(Sub(majorant_spectrum, sigma_a), sigma_s);
        
        // Three-event probabilities
        SampledSpectrum P_s = MulFloat(sigma_s, inv_majorant);
        SampledSpectrum P_a = MulFloat(sigma_a, inv_majorant);
        SampledSpectrum P_n = MulFloat(sigma_n, inv_majorant);
        
        // Per-channel Russian roulette: scattering or absorption both count as collision
        for (int i = 0; i < NSpectrumSamples; i++) {
            float rr_sample = SobolSamplerGet1(sobol_sampler);
            if (rr_sample < P_s.values[i] + P_a.values[i]) {
                tr.values[i] = 0.0f;  // Scattering or absorption terminates
            } else {
                tr.values[i] *= P_n.values[i];  // Null scattering
            }
        }
        
        if (--iter == 0) {
            for (int i = 0; i < NSpectrumSamples; i++) {
                tr.values[i] = 0.0f;
            }
            break;
        }
    }
    
    // PDF calculation
    if (!scattered) {
        // Transmission: PDF = average Tr
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += tr.values[i];
        }
        result.pdf /= float(NSpectrumSamples);
    } else {
        // Scattering: PDF = average Tr * sigma_s
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += tr.values[i] * info.medium.sigma_s.values[i];
        }
        result.pdf /= float(NSpectrumSamples);
    }
    
    result.transmittance = tr;
    
    if (scattered) {
        result.transmittance = Mul(tr, info.medium.sigma_s);
    }
    
    return result;
}

/**
 * @brief Unified medium distance sampling function that dispatches to the appropriate medium model
 * @param info Intersection data containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param ray_direction Ray direction at the intersection point (normalized)
 * @param sobol_sampler Sobol sequence sampler (consumes random samples)
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo MediumDistanceSample(IntersectionInfo info, SampledSpectrum beta, vec3 ray_direction, inout SobolSampler sobol_sampler) {
    MediumSampleInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.scattered = false;
    
    // Dispatch based on medium type
    if (MediumType_Homogeneous == info.medium.type) {
        return HomogeneousDistanceSample(info, beta, sobol_sampler);
    }
    else if (MediumType_Heterogeneous == info.medium.type) {
        return HeterogeneousDistanceSample(info, beta, ray_direction, sobol_sampler);
    }
    
    // Unknown medium type, return zero contribution
    return result;
}

/**
 * @brief Unified medium distance evaluation function that dispatches to the appropriate medium model
 * @param info Intersection data containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @param ray_direction Ray direction at the intersection point (normalized)
 * @param sobol_sampler Sobol sequence sampler (consumes random samples)
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo MediumDistanceEvaluate(IntersectionInfo info, SampledSpectrum beta, bool scattered, vec3 ray_direction, inout SobolSampler sobol_sampler) {
    MediumEvalInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on medium type
    if (MediumType_Homogeneous == info.medium.type) {
        return HomogeneousDistanceEvaluate(info, beta, scattered);
    }
    else if (MediumType_Heterogeneous == info.medium.type) {
        return HeterogeneousDistanceEvaluate(info, beta, scattered, ray_direction, sobol_sampler);
    }
    
    // Unknown medium type, return zero contribution
    return result;
}

#endif