#ifndef _SAMPLER__GLSL__
#define _SAMPLER__GLSL__

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
 * @brief Sobol sequence sampler using quasi-random number generation
 * 
 * Provides low-discrepancy sequence values based on Sobol sequences.
 * Uses dimension-wise sampling for well-distributed sample points.
 */
struct SobolSampler {
    uint index;        ///< Current sequence index
    uint dim;          ///< Current dimension
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

#endif // _SAMPLER__GLSL__