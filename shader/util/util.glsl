#ifndef UTIL_GLSL
#define UTIL_GLSL

// Spectral rendering constants
const int NSpectrumSamples = 16;              // Number of spectral samples per calculation
const float LambdaMin = 360.0f;               // Minimum visible wavelength (nanometers)
const float LambdaMax = 830.0f;               // Maximum visible wavelength (nanometers)
const float CIEYIntegral = 106.856895f;
const int NCIESamples = int(LambdaMax - LambdaMin) + 1; // 471

const float PI = 3.1415926535897932385f;
const float MaxFloat = 3.402823466e+38f;
const float Epsilon = 1e-4f;

/**
 * @struct SampledSpectrum
 * @brief Represents spectral distribution with discrete wavelength samples
 */
struct SampledSpectrum {
    float values[NSpectrumSamples];  // Spectral values at sampled wavelengths
};

struct Medium {
    int phase_type;
    float g;

    int type;
    SampledSpectrum sigma_s;
    SampledSpectrum sigma_t;
};

/**
 * @struct Material
 * @brief Material information structure containing optical properties and texture indices
 */
struct Material {
    int type;

    //SampledSpectrum emission;
    //int emission_texture;

    SampledSpectrum diffuse;
    int diffuse_texture;

    float roughness;
    int roughness_texture;
};

/**
 * @struct IntersectionInfo
 * @brief Ray-geometry intersection information structure
 */
struct IntersectionInfo {
    int tri_index;
	float distance;
	vec3 position;
	vec3 shading_normal;
    vec3 geometry_normal;
	bool front_face;
    vec2 uv;
    Material material;
    Medium medium;
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
	info.front_face = dot(dir, ng) < 0.0f;
	info.geometry_normal = info.front_face ? ng : -ng;
	info.shading_normal = info.front_face ? ns : -ns;
    
    // Check if geometry normal and shading normal are inconsistent (negative dot product)
	if (dot(info.geometry_normal, info.shading_normal) < 0.0f) {
        // If inconsistent, use reflection operation to flip shading normal to the same side as geometry normal
        // This ensures normal direction consistency and prevents lighting calculation errors
		info.shading_normal = reflect(info.shading_normal, info.geometry_normal);
	}

    return info;
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
    return normalize(vec3(dot(vec, right), dot(vec, up), dot(vec, forward)));
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
    return normalize(dir.x * right + dir.y * up + dir.z * forward);
}

/**
 * @brief Converts a unit vector from world space to a local coordinate system defined by an up-direction
 * @param dir Unit vector in world space to transform
 * @param up Up-direction vector in world space (defines local space's vertical axis)
 * @return Transformed vector in local space (orthonormal basis relative to up)
 */
vec3 ToLocalFromUp(vec3 dir, vec3 up) {
    vec3 b, c;
    // Construct orthonormal basis: prioritize stability by comparing |up.x| vs |up.y|
    if (abs(up.x) > abs(up.y)) {
        float len_inv = 1.0f / sqrt(up.x * up.x + up.z * up.z);
        c = vec3(up.z * len_inv, 0.0f, -up.x * len_inv); // Tangent vector (x-z plane)
    } 
    else {
        float len_inv = 1.0f / sqrt(up.y * up.y + up.z * up.z);
        c = vec3(0.0f, up.z * len_inv, -up.y * len_inv); // Tangent vector (y-z plane)
    }
    b = cross(c, up); // Complete orthonormal basis: B = C × up

    // Transform: project world-space dir onto local basis {B, C, up}
    return normalize(vec3(dot(dir, b), dot(dir, c), dot(dir, up)));
}

/**
 * @brief Converts a unit vector from local space (defined by up-direction) back to world space
 * @param dir Unit vector in local space to transform
 * @param up Up-direction vector in world space (same as used in ToLocalFromUp)
 * @return Unit vector in world space
 */
vec3 ToWorldFromUp(vec3 dir, vec3 up) {
    vec3 b, c;
    // Reconstruct identical orthonormal basis as ToLocalFromUp
    if (abs(up.x) > abs(up.y)) {
        float len_inv = 1.0f / sqrt(up.x * up.x + up.z * up.z);
        c = vec3(up.z * len_inv, 0.0f, -up.x * len_inv);
    } 
    else {
        float len_inv = 1.0f / sqrt(up.y * up.y + up.z * up.z);
        c = vec3(0.0f, up.z * len_inv, -up.y * len_inv);
    }
    b = cross(c, up);

    // Reconstruct world-space vector: linear combination of basis vectors
    vec3 world_vec = dir.x * b + dir.y * c + dir.z * up;

    return normalize(world_vec); // Ensure output remains unit length
}

#endif // UTIL_GLSL