#ifndef _RANDOM_SAMPLER__GLSL__
#define _RANDOM_SAMPLER__GLSL__

uniform usamplerBuffer SobolMatricesTable;

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
    uint pixel_x;       ///< Current pixel x-coordinate
    uint pixel_y;       ///< Current pixel y-coordinate
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
 * @param sampler IndependentSampler instance (seed will be modified via inout reference)
 * @return Random float value between 0 (inclusive) and 1 (exclusive)
 */
float IndependentSamplerGet1(inout IndependentSampler sampler) {
    return float(WangHash(sampler.seed)) / 4294967296.0f;
}

/**
 * @brief Sets current pixel coordinates and updates seeds accordingly
 * @param sampler IndependentSampler instance to modify (passed as inout reference)
 * @param x Pixel x-coordinate in screen space
 * @param y Pixel y-coordinate in screen space
 */
void IndependentSamplerSetPixel(inout IndependentSampler sampler, uint x, uint y) {
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
}

/**
 * @brief Advances to the next sample by incrementing sample index
 * @param sampler IndependentSampler instance to modify (passed as inout reference)
 */
void IndependentSamplerNextSample(inout IndependentSampler sampler) {
    sampler.sample_index++;
    sampler.frame_count++;
    
    // Update main seed with sample index for variation
    sampler.seed = uint(
        uint(sampler.pixel_x) * uint(1973) + 
        uint(sampler.pixel_y) * uint(9277) + 
        (sampler.frame_count + sampler.sample_index) * uint(26699)) | uint(1);
}

/**
 * @brief Advances multiple samples at once
 * @param sampler IndependentSampler instance to modify (passed as inout reference)
 * @param count Number of samples to advance
 */
void IndependentSamplerNextSamples(inout IndependentSampler sampler, uint count) {
    for (int i = 0; i < count; i++) {
        IndependentSamplerNextSample(sampler);
    }
}

/**
 * @brief Sobol sequence sampler using quasi-random number generation
 * 
 * Provides low-discrepancy sequence values based on Sobol sequences.
 * Uses dimension-wise sampling for well-distributed sample points.
 */
struct SobolSampler {
    uint index;        ///< Current sequence index
    uint dim;           ///< Current dimension
    uint seed;         ///< Seed for randomization
    uint scramble;     ///< Scrambling value for randomization
};

/**
 * @brief Initializes a SobolSampler instance with default values
 * @param seed Seed value for randomization
 * @return Newly initialized SobolSampler structure
 */
SobolSampler SobolSamplerNew(uint seed) {
    SobolSampler sampler;
    sampler.index = 0;
    sampler.dim = 0;
    sampler.seed = seed;
    sampler.scramble = seed;
    
    return sampler;
}

/**
 * @brief Sobol sequence sample generation function
 * @param index Sequence index
 * @param dim Dimension index
 * @param scramble Scrambling value (default = 0)
 * @return Sobol sample value
 */
uint SobolSample(uint index, uint dim, uint scramble) {
    uint r = scramble;
    
    // Sobol matrices would be defined as a constant array
    // For simplicity, we assume SobolMatrices is available as a uniform buffer
    for (int i = int(dim) * 52; 0 != index; index >>= 1, i++) {
        if (0 != (index & 1)) {
            r ^= texelFetch(SobolMatricesTable, i).r;
        }
    }
    
    return r;
}

/**
 * @brief Generates a random float in range [0, 1) using Sobol sequence
 * @param sampler SobolSampler instance (dim will be incremented via inout reference)
 * @return Random float value between 0 (inclusive) and 1 (exclusive)
 */
float SobolSamplerGet1(inout SobolSampler sampler) {
    float r = float(SobolSample(sampler.index, sampler.dim, sampler.scramble)) / 4294967296.0f;
    sampler.dim++;
    
    return r;
}

/**
 * @brief Sets current pixel coordinates and resets dimension counter
 * @param sampler SobolSampler instance to modify (passed as inout reference)
 * @param x Pixel x-coordinate in screen space
 * @param y Pixel y-coordinate in screen space
 */
void SobolSamplerSetPixel(inout SobolSampler sampler, uint x, uint y) {
    sampler.dim = 0;
    
    // Generate scramble value based on pixel coordinates
    // Using a simple hash function for demonstration
    // In practice, you might want to use a more sophisticated method
    uint hash = x ^ (y << 16);
    sampler.scramble = WangHash(hash) ^ sampler.seed;
}

/**
 * @brief Advances to the next sample by incrementing index and resetting dimension
 * @param sampler SobolSampler instance to modify (passed as inout reference)
 */
void SobolSamplerNextSample(inout SobolSampler sampler) {
    sampler.index++;
    sampler.dim = 0;
}

/**
 * @brief Advances multiple samples at once
 * @param sampler SobolSampler instance to modify (passed as inout reference)
 * @param count Number of samples to advance
 */
void SobolSamplerNextSamples(inout SobolSampler sampler, uint count) {
    sampler.index += count;
    sampler.dim = 0;
}

#endif // _RANDOM_SAMPLER__GLSL__