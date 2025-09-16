#ifndef _SAMPLER__GLSL__
#define _SAMPLER__GLSL__

/**
 * @brief Wang hash function for random number generation
 * @param seed Seed value (will be modified during computation)
 * @return Hash value based on input seed
 */
uint WangHash(uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);

    return seed;
}

/**
 * @brief Independent sampler using Wang hash for random number generation
 * 
 * Provides deterministic random values based on pixel coordinates and sample index.
 * Uses Wang hash algorithm for efficient GPU-friendly random number generation.
 */
struct IndependentSampler {
    uint seed;         ///< Current seed state for random number generation
    uint seed_sync;    ///< Synchronized seed state for consistent random values
    int pixel_x;       ///< Current pixel x-coordinate
    int pixel_y;       ///< Current pixel y-coordinate
    uint frame_count;  ///< Frame counter for temporal variation
    uint sample_index; ///< Current sample index within pixel
};

/**
 * @brief Initializes an IndependentSampler instance with default values
 * @return Newly initialized IndependentSampler structure
 */
IndependentSampler IndependentSamplerNew() {
    IndependentSampler sampler;
    sampler.pixel_x = 0;
    sampler.pixel_y = 0;
    sampler.frame_count = 0;
    sampler.sample_index = 0;
    sampler.seed = 1;
    sampler.seed_sync = 1;

    return sampler;
}

/**
 * @brief Generates a random float in range [0, 1) using the main seed
 * @param sampler IndependentSampler instance (seed will be modified)
 * @return Random float value between 0 (inclusive) and 1 (exclusive)
 */
float IndependentSamplerGet1(IndependentSampler sampler) {
    return float(WangHash(sampler.seed)) / 4294967296.0f;
}

/**
 * @brief Sets current pixel coordinates and updates seeds accordingly
 * @param sampler IndependentSampler instance to modify
 * @param x Pixel x-coordinate in screen space
 * @param y Pixel y-coordinate in screen space
 * @return Modified IndependentSampler instance
 */
IndependentSampler IndependentSamplerSetPixel(IndependentSampler sampler, int x, int y) {
    sampler.pixel_x = x;
    sampler.pixel_y = y;
    sampler.sample_index = 0;
    
    // Update main seed based on pixel coordinates and frame count
    sampler.seed = uint(
        uint(x) * uint(1973) + 
        uint(y) * uint(9277) + 
        sampler.frame_count * uint(26699)) | uint(1);
    
    // Update synchronized seed (frame-independent for consistent results)
    sampler.seed_sync = uint(
        uint(x) * uint(1973) + 
        uint(y) * uint(9277) + 
        uint(114514) * uint(26699)) | uint(1);

    return sampler;
}

/**
 * @brief Advances to the next sample by incrementing sample index
 * @param sampler IndependentSampler instance to modify
 * @return Modified IndependentSampler instance with updated seed
 */
IndependentSampler IndependentSamplerNextSample(IndependentSampler sampler) {
    sampler.sample_index++;
    sampler.frame_count++;
    
    // Update main seed with sample index for variation
    sampler.seed = uint(
        uint(sampler.pixel_x) * uint(1973) + 
        uint(sampler.pixel_y) * uint(9277) + 
        (sampler.frame_count + sampler.sample_index) * uint(26699)) | uint(1);

    return sampler;
}

/**
 * @brief Advances multiple samples at once
 * @param sampler IndependentSampler instance to modify
 * @param count Number of samples to advance
 * @return Modified IndependentSampler instance after advancement
 */
IndependentSampler IndependentSamplerNextSamples(IndependentSampler sampler, uint count) {
    for (int i = 0; i < count; i++) {
        sampler = IndependentSamplerNextSample(sampler);
    }

    return sampler;
}

#endif // _SAMPLER__GLSL__