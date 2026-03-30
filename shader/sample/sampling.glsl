#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

/**
 * Helper function to sample a 1D alias table with an offset into a larger TBO
 * @param alias_table TBO containing alias table data
 * @param table_size Size of the alias table to sample
 * @param offset Offset into the TBO where this table begins
 * @param sum_distrib Sum of the probability distribution
 * @param sample_xy Random sample in [0, 1] range (x for index, y for probability check)
 * @return Sampled index from the alias table
 */
int AliasTable1DSample(samplerBuffer alias_table, int table_size, int offset, float sum_distrib, vec2 sample_xy) {
    // Calculate index from first random dimension
    int rx = int(sample_xy.x * float(table_size));
    
    // Handle edge case where sample.x is exactly 1.0
    if (rx == table_size) {
        rx = table_size - 1;
    }
    
    // Get alias table data for this index (with offset)
    vec2 table_data = texelFetch(alias_table, offset + rx).xy;
    int alias_index = int(table_data.x);
    float prob = table_data.y;
    
    // Determine which value to return based on second random dimension
    if (sample_xy.y <= prob / sum_distrib) {
        return rx;
    } 
    else {
        return alias_index;
    }
}

/**
 * Samples a 2D alias table using the provided random samples
 * @param col_alias_table TBO containing column alias table data
 * @param col_table_size Size of the column alias table
 * @param col_sum_distrib Sum of the column probability distribution
 * @param row_alias_table TBO containing all row alias table data (concatenated)
 * @param row_table_size Size of each row alias table
 * @param row_sum_distrib Sum of the row probability distribution
 * @param sample_xy1 First random sample for column selection
 * @param sample_xy2 Second random sample for row selection
 * @return 2D coordinates (x, y) of the sampled element
 */
ivec2 AliasTable2DSample(samplerBuffer col_alias_table, int col_table_size, float col_sum_distrib,
                           samplerBuffer row_alias_table, int row_table_size, float row_sum_distrib,
                           vec2 sample_xy1, vec2 sample_xy2) {
    // First sample the column table to get a row index
    int row = AliasTable1DSample(col_alias_table, col_table_size, 0, col_sum_distrib, sample_xy1);
    
    // Calculate offset into the row tables TBO for this specific row
    int row_table_offset = row * row_table_size;
    
    // Sample the specific row table to get the column index
    int col = AliasTable1DSample(row_alias_table, row_table_size, row_table_offset, row_sum_distrib, sample_xy2);
    
    return ivec2(col, row);
}

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

/**
 * @brief Generates a uniform direction on the sphere
 * @param u1 First random number in [0,1)
 * @param u2 Second random number in [0,1)
 * @return vec3 Sampled direction vector
 */
vec3 UniformSphereSample(vec2 sample_xy) {
    float u1 = sample_xy.x;
    float u2 = sample_xy.y;
    
    // 1. Sample the cosine of the polar angle uniformly in [-1, 1]
    float z = 1.0f - 2.0f * u1;
    
    // 2. Sample the azimuthal angle uniformly in [0, 2π]
    float phi = 2.0f * PI * u2;
    
    // 3. Calculate radius in xy-plane
    float r = sqrt(max(0.0f, 1.0f - z * z));
    
    // 4. Convert spherical to Cartesian coordinates
    float x = r * cos(phi);
    float y = r * sin(phi);
    
    return normalize(vec3(x, y, z));
}

/**
 * @brief Computes the probability density function (PDF) for uniform sphere sampling
 * @return float Probability density value (inverse steradians, sr⁻¹)
 */
float UniformSpherePDF() {
    // Uniform sphere sampling PDF is constant: 1/(4π) ≈ 0.079577
    return 1.0f / (4.0f * PI);
}

#endif // SAMPLING_GLSL