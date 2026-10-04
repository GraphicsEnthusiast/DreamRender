#ifndef MEDIUM_GLSL
#define MEDIUM_GLSL

#include "medium/phase_function.glsl"

uniform samplerBuffer DensityData; ///< Global density data for heterogeneous media

// Medium type enumeration, consistent with C++ side
const int MediumType_Homogeneous = 0;   ///< Homogeneous medium
const int MediumType_Heterogeneous = 1; ///< Heterogeneous medium

// Fixed iteration depth for heterogeneous medium tracking
const int MaxMediumIterations = 2048;

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
 * @brief Get density value at normalized position using trilinear interpolation
 * @param medium Medium data containing density grid info
 * @param p Normalized position in [0,1]^3
 * @return Interpolated density value
 */
float GetDensity(Medium medium, vec3 p) {
    // Scale to grid coordinates
    vec3 ps = p * vec3(medium.density_resolution);
    vec3 psi = floor(ps);
    vec3 delta = ps - psi;
    
    // Clamp base indices to valid range
    ivec3 base = ivec3(psi);
    base = max(base, ivec3(0));
    base = min(base, medium.density_resolution - ivec3(1));
    
    // Compute neighbor indices with clamping
    ivec3 x1 = min(base + ivec3(1, 0, 0), medium.density_resolution - ivec3(1));
    ivec3 y1 = min(base + ivec3(0, 1, 0), medium.density_resolution - ivec3(1));
    ivec3 z1 = min(base + ivec3(0, 0, 1), medium.density_resolution - ivec3(1));
    ivec3 xy1 = min(base + ivec3(1, 1, 0), medium.density_resolution - ivec3(1));
    ivec3 xz1 = min(base + ivec3(1, 0, 1), medium.density_resolution - ivec3(1));
    ivec3 yz1 = min(base + ivec3(0, 1, 1), medium.density_resolution - ivec3(1));
    ivec3 xyz1 = min(base + ivec3(1, 1, 1), medium.density_resolution - ivec3(1));
    
    // Fetch density values at 8 corners
    // Index formula: offset + z * ny * nx + y * nx + x
    float d000 = texelFetch(DensityData, medium.density_offset + base.z * medium.density_resolution.y * medium.density_resolution.x + base.y * medium.density_resolution.x + base.x).r;
    float d100 = texelFetch(DensityData, medium.density_offset + x1.z * medium.density_resolution.y * medium.density_resolution.x + base.y * medium.density_resolution.x + x1.x).r;
    float d010 = texelFetch(DensityData, medium.density_offset + y1.z * medium.density_resolution.y * medium.density_resolution.x + y1.y * medium.density_resolution.x + base.x).r;
    float d110 = texelFetch(DensityData, medium.density_offset + xy1.z * medium.density_resolution.y * medium.density_resolution.x + xy1.y * medium.density_resolution.x + xy1.x).r;
    float d001 = texelFetch(DensityData, medium.density_offset + z1.z * medium.density_resolution.y * medium.density_resolution.x + base.y * medium.density_resolution.x + base.x).r;
    float d101 = texelFetch(DensityData, medium.density_offset + xz1.z * medium.density_resolution.y * medium.density_resolution.x + base.y * medium.density_resolution.x + xz1.x).r;
    float d011 = texelFetch(DensityData, medium.density_offset + yz1.z * medium.density_resolution.y * medium.density_resolution.x + yz1.y * medium.density_resolution.x + base.x).r;
    float d111 = texelFetch(DensityData, medium.density_offset + xyz1.z * medium.density_resolution.y * medium.density_resolution.x + xyz1.y * medium.density_resolution.x + xyz1.x).r;
    
    // Trilinear interpolation
    float d00 = mix(d000, d100, delta.x);
    float d10 = mix(d010, d110, delta.x);
    float d01 = mix(d001, d101, delta.x);
    float d11 = mix(d011, d111, delta.x);
    
    float d0 = mix(d00, d10, delta.y);
    float d1 = mix(d01, d11, delta.y);
    
    return mix(d0, d1, delta.z);
}

/**
 * @brief Evaluate transmittance and PDF using delta tracking
 * @param medium Medium data
 * @param origin Ray origin (intersection point)
 * @param ray_dir Ray direction (normalized)
 * @param tmax Maximum distance
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @param sobol_sampler Sobol sampler (consumes random samples)
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo DeltaTrackingEvaluate(Medium medium, vec3 origin, vec3 ray_dir, float tmax, SampledSpectrum beta, bool scattered, inout SobolSampler sobol_sampler) {
    MediumEvalInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Compute wavelength-independent majorant
    float max_sigma_t = Max(medium.sigma_t);
    float majorant = max_sigma_t * (1.0f / medium.inv_max_density);
    float inv_majorant = 1.0f / majorant;
    
    // Sample wavelength channel for transmittance evaluation
    SampledSpectrum albedo = Div(medium.sigma_s, medium.sigma_t);
    float wavelength_sample = SobolSamplerGet1(sobol_sampler);
    WavelengthSampleInfo wavelength_result = MediumWavelengthSample(beta, albedo, wavelength_sample);
    int channel = wavelength_result.channel;
    SampledSpectrum wavelength_pmf = wavelength_result.pmf;
    
    // Initialize transmittance for all channels
    SampledSpectrum trans = SampledSpectrumNewFloat(1.0f);
    float dist = 0.0f;
    
    for (int iter = 0; iter < MaxMediumIterations; iter++) {
        // Sample collision-free distance
        float u = SobolSamplerGet1(sobol_sampler);
        dist += -log(1.0f - u) * inv_majorant;
        
        if (dist >= tmax) {
            break;
        }
        
        // Compute point in medium
        vec3 p = origin + ray_dir * dist;
        
        // Normalize to [0,1]^3
        vec3 p_norm = (p - medium.density_min) / (medium.density_max - medium.density_min);
        
        // Check if point is inside the medium bounding box
        if (p_norm.x >= 0.0f && p_norm.x <= 1.0f &&
            p_norm.y >= 0.0f && p_norm.y <= 1.0f &&
            p_norm.z >= 0.0f && p_norm.z <= 1.0f) {
            
            // Get density at point
            float density = GetDensity(medium, p_norm);
            
            // Compute sigma_a, sigma_s, sigma_n at this point
            SampledSpectrum sigma_s = MulFloat(medium.sigma_s, density);
            SampledSpectrum sigma_t = MulFloat(medium.sigma_t, density);
            SampledSpectrum sigma_a = Sub(sigma_t, sigma_s);
            SampledSpectrum sigma_n = Sub(Sub(SampledSpectrumNewFloat(majorant), sigma_a), sigma_s);
            
            // Russian roulette: P_n = sigma_n / majorant
            // Collision if random > P_n[channel]
            if (SobolSamplerGet1(sobol_sampler) > sigma_n.values[channel] * inv_majorant) {
                return result;
            }
            
            // No collision, update transmittance: trans *= sigma_n / sigma_n[channel]
            trans = Mul(trans, DivFloat(sigma_n, sigma_n.values[channel]));
        }
    }
    
    // Calculate PDF based on scattering status
    if (!scattered) {
        // Transmission case: PDF = Tr(distance)
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i];
        }
    }
    else {
        // Scattering case: PDF = σ_t * Tr(distance)
        for (int i = 0; i < NSpectrumSamples; i++) {
            result.pdf += wavelength_pmf.values[i] * trans.values[i] * medium.sigma_t.values[i];
        }
    }
    
    result.transmittance = trans;
    
    // Apply scattering coefficient if scattered
    if (scattered) {
        result.transmittance = Mul(result.transmittance, medium.sigma_s);
    }
    
    return result;
}

/**
 * @brief Sample a scattering distance using delta tracking
 * @param medium Medium data
 * @param origin Ray origin (intersection point)
 * @param ray_dir Ray direction (normalized)
 * @param tmax Maximum distance
 * @param beta Spectral beta (path contribution)
 * @param sobol_sampler Sobol sampler (consumes random samples)
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo DeltaTrackingSample(Medium medium, vec3 origin, vec3 ray_dir, float tmax, SampledSpectrum beta, inout SobolSampler sobol_sampler) {
    MediumSampleInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.scattered = false;
    
    // Sample wavelength channel
    SampledSpectrum albedo = Div(medium.sigma_s, medium.sigma_t);
    float wavelength_sample = SobolSamplerGet1(sobol_sampler);
    WavelengthSampleInfo wavelength_result = MediumWavelengthSample(beta, albedo, wavelength_sample);
    int channel = wavelength_result.channel;
    SampledSpectrum wavelength_pmf = wavelength_result.pmf;
    
    // Use the sampled channel's sigma_t as scalar sigma
    float sigma = medium.sigma_t.values[channel];
    float inv_majorant = medium.inv_max_density / sigma;
    
    float dist = 0.0f;
    
    for (int iter = 0; iter < MaxMediumIterations; iter++) {
        // Sample collision-free distance
        float u = SobolSamplerGet1(sobol_sampler);
        dist += -log(1.0f - u) * inv_majorant;
        
        if (dist >= tmax) {
            break;
        }
        
        // Compute point in medium
        vec3 p = origin + ray_dir * dist;
        
        // Normalize to [0,1]^3
        vec3 p_norm = (p - medium.density_min) / (medium.density_max - medium.density_min);
        
        // Check if point is inside the medium bounding box
        if (p_norm.x >= 0.0f && p_norm.x <= 1.0f &&
            p_norm.y >= 0.0f && p_norm.y <= 1.0f &&
            p_norm.z >= 0.0f && p_norm.z <= 1.0f) {
            
            // Get density at point
            float density = GetDensity(medium, p_norm);
            
            // Collision test: density * invMaxDensity > random
            if (density * medium.inv_max_density > SobolSamplerGet1(sobol_sampler)) {
                // Scattering occurs
                result.distance = dist;
                result.scattered = true;
                
                // Transmittance at scattering point
                float tr = exp(-sigma * dist);
                SampledSpectrum trans = SampledSpectrumNewFloat(tr);
                
                // PDF = σ_t * Tr(distance)
                for (int i = 0; i < NSpectrumSamples; i++) {
                    result.pdf += wavelength_pmf.values[i] * trans.values[i] * medium.sigma_t.values[i];
                }
                
                result.transmittance = Mul(trans, medium.sigma_s);

                return result;
            }
        }
    }
    
    // Transmission
    result.distance = tmax;
    result.scattered = false;
    
    // Transmittance at tmax
    float tr = exp(-sigma * tmax);
    SampledSpectrum trans = SampledSpectrumNewFloat(tr);
    
    for (int i = 0; i < NSpectrumSamples; i++) {
        result.pdf += wavelength_pmf.values[i] * trans.values[i];
    }
    
    result.transmittance = trans;

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
 * @brief Evaluate transmittance for heterogeneous medium using delta tracking
 * @param info Intersection info containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @param ray_dir Ray direction (normalized)
 * @param sobol_sampler Sobol sampler
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo HeterogeneousDistanceEvaluate(IntersectionInfo info, SampledSpectrum beta, bool scattered, vec3 ray_dir, inout SobolSampler sobol_sampler) {
    return DeltaTrackingEvaluate(info.medium, info.position, ray_dir, info.distance, beta, scattered, sobol_sampler);
}

/**
 * @brief Sample a distance in heterogeneous medium using delta tracking
 * @param info Intersection info containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param ray_dir Ray direction (normalized)
 * @param sobol_sampler Sobol sequence sampler
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo HeterogeneousDistanceSample(IntersectionInfo info, SampledSpectrum beta, vec3 ray_dir, inout SobolSampler sobol_sampler) {
    return DeltaTrackingSample(info.medium, info.position, ray_dir, info.distance, beta, sobol_sampler);
}

/**
 * @brief Unified medium distance evaluation function that dispatches to the appropriate medium model
 * @param info Intersection data containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param scattered Whether scattering is assumed
 * @param ray_dir Ray direction (normalized, needed for heterogeneous media)
 * @param sobol_sampler Sobol sequence sampler
 * @return MediumEvalInfo containing transmittance and PDF
 */
MediumEvalInfo MediumDistanceEvaluate(IntersectionInfo info, SampledSpectrum beta, bool scattered, vec3 ray_dir, inout SobolSampler sobol_sampler) {
    MediumEvalInfo result;
    result.transmittance = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on medium type
    if (MediumType_Homogeneous == info.medium.type) {
        return HomogeneousDistanceEvaluate(info, beta, scattered);
    }
    else if (MediumType_Heterogeneous == info.medium.type) {
        return HeterogeneousDistanceEvaluate(info, beta, scattered, ray_dir, sobol_sampler);
    }
    
    // Unknown medium type, return zero contribution
    return result;
}

/**
 * @brief Unified medium distance sampling function that dispatches to the appropriate medium model
 * @param info Intersection data containing medium properties
 * @param beta Spectral beta (path contribution)
 * @param ray_dir Ray direction (normalized, needed for heterogeneous media)
 * @param sobol_sampler Sobol sequence sampler
 * @return MediumSampleInfo containing transmittance, distance, PDF, and scattering status
 */
MediumSampleInfo MediumDistanceSample(IntersectionInfo info, SampledSpectrum beta, vec3 ray_dir, inout SobolSampler sobol_sampler) {
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
        return HeterogeneousDistanceSample(info, beta, ray_dir, sobol_sampler);
    }
    
    // Unknown medium type, return zero contribution
    return result;
}

#endif