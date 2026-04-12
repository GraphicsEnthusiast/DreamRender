#ifndef FILTER_GLSL
#define FILTER_GLSL

// The following comments are from lajolla:
// Many common open source renderers implement pixel filtering using
// a "splatting" approach: they sample a point from a pixel, and then
// splat the contribution to all nearby pixels overlapped with the
// filter support.
// This approach works fine, but has a few disadvantages:
// 1) This introduces race conditions between different pixels, and requires atomic operations.
// 2) This introduces correlation between pixels and hurts denoising.
// 3) The splatting approach is biased and creates artifacts at low sampling rates.
// 4) For filters with infinite supports (e.g., Gaussian), 
//    this requires a discontinuous cutoff radius (otherwise it would be too slow).
// For these reasons, many modern production renderers have started to employ
// a different and simpler strategy.
// For each pixel, we solve for the pixel filter integral by directly importance
// sample that filter, and we *do not* share samples among pixels.
// This allows us to avoid all three problems above.
// This approach was described by Shirley et al. in 1991
// "A ray tracing framework for global illumination systems"
// and was discussed more recently by Ernst et al. in "Filter Importance Sampling".

// To make things simple, we only support filters with closed-form importance
// sampling distribution. This means that our sampling weight is always 1, since
// a pixel filter always normalize to 1.

/**
 * @struct FilterEvaluateInfo
 * @brief Structure containing all information about a filter
 */
struct FilterEvaluateInfo {
    float weight;
    float pdf;
};

/**
 * @struct FilterSampleInfo
 * @brief Structure containing all information about a filter sample
 */
struct FilterSampleInfo {
    vec2 offset;
    float weight;
    float pdf;
};

/**
 * @brief Evaluates a 2D Gaussian filter at a given offset
 * @param filter_stddev Standard deviation of the Gaussian filter (controls width)
 * @param offset Offset from the pixel center
 * @return FilterSampleInfo containing the same offset, weight, and PDF
 */
FilterEvaluateInfo GaussianFilterEvaluate(float filter_stddev, vec2 offset) {
    // Compute Gaussian filter weight at the given offset
    float r_squared = dot(offset, offset);
    float variance = filter_stddev * filter_stddev;
    float exponent = -0.5f * r_squared / variance;
    float normalization = 1.0f / (2.0f * PI * variance);
    float weight = exp(exponent) * normalization;
    
    // For importance sampling of the filter itself, the PDF equals the filter weight
    float pdf = weight;
    
    FilterEvaluateInfo result;
    result.weight = weight;
    result.pdf = pdf;
    
    return result;
}

/**
 * @brief Samples a 2D Gaussian filter using the Box-Muller transform
 * @param filter_stddev Standard deviation of the Gaussian filter (controls width)
 * @param sample_xy Two independent uniform random numbers in [0,1]²
 * @return FilterSampleInfo containing offset, weight, and PDF
 */
FilterSampleInfo GaussianFilterSample(float filter_stddev, vec2 sample_xy) {
    // Avoid log(0) by clamping the first random number
    float r1 = max(sample_xy.x, Epsilon);
    
    // Box-Muller transform: convert uniform to Gaussian distribution
    // Generate radial distance from Rayleigh distribution
    float r = filter_stddev * sqrt(-2.0f * log(r1));
    
    // Generate uniform angle
    float theta = 2.0f * PI * sample_xy.y;
    
    // Convert polar coordinates to Cartesian
    vec2 offset = vec2(r * cos(theta), r * sin(theta));
    
    // Compute Gaussian filter weight at the sampled offset
    // This is the value of the reconstruction filter at this point
    float r_squared = dot(offset, offset);
    float variance = filter_stddev * filter_stddev;
    float exponent = -0.5f * r_squared / variance;
    float normalization = 1.0f / (2.0f * PI * variance);
    float weight = exp(exponent) * normalization;
    
    // For importance sampling of the filter itself, the PDF equals the filter weight
    float pdf = weight;
    
    FilterSampleInfo result;
    result.offset = offset; // Sampled offset relative to pixel center in [-0.5, 0.5] range
    result.weight = weight;
    result.pdf = pdf;
    
    return result;
}

#endif // FILTER_GLSL