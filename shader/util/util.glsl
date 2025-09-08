#ifndef _UTIL__GLSL__
#define _UTIL__GLSL__

// Spectral rendering constants
const int NSpectrumSamples = 8;               // Number of spectral samples per calculation
const float LambdaMin = 360.0f;               // Minimum visible wavelength (nanometers)
const float LambdaMax = 830.0f;               // Maximum visible wavelength (nanometers)
const float CIEYIntegral = 106.856895f;
const int NCIESamples = int(LambdaMax - LambdaMin) + 1; // 471

const float PI = 3.1415926535897932385f;
const float MaxFloat = 3.402823466e+38f;

/**
 * @struct SampledSpectrum
 * @brief Represents spectral distribution with discrete wavelength samples
 */
struct SampledSpectrum {
    float values[NSpectrumSamples];  // Spectral values at sampled wavelengths
};

/**
 * @struct MaterialInfo
 * @brief Material information structure containing optical properties and texture indices
 */
struct MaterialInfo {
    SampledSpectrum diffuse;
    int diffuse_texture;

    SampledSpectrum roughness;
    int roughness_texture;
};

/**
 * @struct IntersectionInfo
 * @brief Ray-geometry intersection information structure
 */
struct IntersectionInfo {
	float distance;
	vec3 position;
	vec3 shading_normal;
    vec3 geometry_normal;
	bool front_face;
    MaterialInfo material_info;
};

/**
 * @brief Sets and adjusts normal information at intersection point
 * @param info Initial intersection information structure
 * @param dir Ray direction vector (normalized)
 * @param ng Geometry normal (original normal of geometric surface)
 * @param ns Shading normal (potentially modified or interpolated normal)
 * @return IntersectionInfo Updated intersection information structure
 */
IntersectionInfo SetNormal(IntersectionInfo info, vec3 dir, vec3 ng, vec3 ns) {
    IntersectionInfo new_info = info;

	new_info.front_face = dot(dir, ng) < 0.0f;
	new_info.geometry_normal = new_info.front_face ? ng : -ng;
	new_info.shading_normal = new_info.front_face ? ns : -ns;
    
    // Check if geometry normal and shading normal are inconsistent (negative dot product)
	if (dot(new_info.geometry_normal, new_info.shading_normal) < 0.0f) {
        // If inconsistent, use reflection operation to flip shading normal to the same side as geometry normal
        // This ensures normal direction consistency and prevents lighting calculation errors
		new_info.shading_normal = reflect(new_info.shading_normal, new_info.geometry_normal);
	}

    return new_info;
}

/**
 * @brief Converts a vector from world space to local space
 * @param vec Vector to transform
 * @param right Local space right vector
 * @param up Local space up vector
 * @param forward Local space forward vector
 * @return Transformed vector in local space
 */
vec3 ToLocal(vec3 vec, vec3 right, vec3 up, vec3 forward) {
    return vec3(dot(vec, right), dot(vec, up), dot(vec, forward));
}

/**
 * @brief Converts a vector from local space (defined by basis vectors) to world space
 * @param dir Vector in local space to transform
 * @param right Local space right vector (typically the x-axis basis)
 * @param up Local space up vector (typically the y-axis basis)
 * @param forward Local space forward vector (typically the z-axis basis)
 * @return Transformed vector in world space
 */
vec3 ToWorld(vec3 dir, vec3 right, vec3 up, vec3 forward) {
    return dir.x * right + dir.y * up + dir.z * forward;
}

#endif // _UTIL__GLSL__
