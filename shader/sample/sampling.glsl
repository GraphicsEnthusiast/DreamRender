#ifndef _SAMPLING__GLSL__
#define _SAMPLING__GLSL__

/**
 * @brief Samples a direction on the unit hemisphere using uniform disk sampling with cosine weighting 
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @return vec3 Unit vector in local coordinate system (Z-up hemisphere)
 */
vec3 CosineHemisphereSample(vec2 sample_xy) {
    // 1. Uniform disk sampling using polar coordinates
    float r = sqrt(sample_xy.x);       // Apply inverse CDF for radial distribution
    float phi = 2.0f * PI * sample_xy.y; // Uniform azimuthal angle [0, 2π]
    
    // Convert polar to Cartesian coordinates on unit disk
    vec3 p;
    p.x = r * cos(phi);
    p.y = r * sin(phi);
    
    // 2. Project disk point onto hemisphere (Z > 0)
    // Uses Pythagorean theorem to ensure unit length: x² + y² + z² = 1
    p.z = sqrt(max(0.0f, 1.0f - p.x * p.x - p.y * p.y));
    
    // 3. Return normalized direction (mathematically redundant but ensures numerical stability)
    return normalize(p);
}

/**
 * @brief Computes the probability density function (PDF) for cosine-weighted hemisphere sampling
 * @param n_dot_l Cosine of the angle between surface normal and light direction (clamped to [0,1])
 * @return float Probability density value (inverse steradians, sr⁻¹)
 */
float CosineHemispherePDF(float n_dot_l) {
	return max(0.0, n_dot_l) / PI;
}

#endif // _SAMPLING__GLSL__