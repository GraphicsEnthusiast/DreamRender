#ifndef UTIL_GLSL
#define UTIL_GLSL

// Spectral rendering constants
const int NSpectrumSamples = 4;               // Number of spectral samples per calculation
const float LambdaMin = 360.0f;               // Minimum visible wavelength (nanometers)
const float LambdaMax = 830.0f;               // Maximum visible wavelength (nanometers)
const float CIEYIntegral = 106.856895f;
const int NCIESamples = int(LambdaMax - LambdaMin) + 1; // 471

const float PI = 3.1415926535897932385f;
const float MaxFloat = 3.402823466e+38f;
const float Epsilon = 1e-4f;

const int TransportMode_Radiance = 0;   // Radiance transport mode: light travels from the eye to light sources (path tracing)
const int TransportMode_Importance = 1; // Importance transport mode: light travels from light sources to the eye (light tracing)

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

    // Emission
    SampledSpectrum emission;
    int emission_texture;

    // Diffuse
    SampledSpectrum diffuse;
    int diffuse_texture;

    // Anisotropic roughness (x = U, y = V)
    float roughness_u;
    float roughness_v;
    int roughness_aniso_texture_u;
    int roughness_aniso_texture_v;

    // Specular color (for conductor / metals)
    SampledSpectrum specular;
    int specular_texture;

    // Conductor complex IOR - real part (eta) - constant only
    SampledSpectrum eta;

    // Conductor complex IOR - imaginary part (k) - constant only
    SampledSpectrum k;

    // Index of refraction for the interior medium
    float in_ior;
    // Index of refraction for the exterior medium
    float out_ior;
};

/**
 * @struct IntersectionInfo
 * @brief Ray-geometry intersection information structure
 */
struct IntersectionInfo {
    bool is_light;
    int tri_index;
	float distance;
	vec3 position;
	vec3 shading_normal;
    vec3 geometry_normal;
	bool front_face;
    vec2 uv;
    bool has_medium;
    Material material;
    Medium medium;
    int transport_mode;
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

/**
 * @brief Ray structure for ray tracing parameters
 */
struct Ray {
    vec3 origin;    ///< Ray origin point
    vec3 direction; ///< Ray direction vector (normalized)
    float tmin;     ///< Minimum ray distance (avoid self-intersection)
    float tmax;     ///< Maximum ray distance
    int transport_mode; ///< Transport mode for ray tracing (radiance or importance)
};

const float OriginScale = 1.0f / 32.0f;
const float FloatScale = 1.0f / 65536.0f;
const float IntScale = 256.0f;

/**
 * @brief Computes the offset ray origin to avoid self-intersection
 * @param p Original intersection point
 * @param n Surface normal (points outward for rays exiting the surface, else is flipped)
 * @return Offset point
 */
vec3 OffsetRayOrigin(vec3 p, vec3 n) {
    // Compute integer offset scaled by int_scale
    ivec3 of_i = ivec3(int(IntScale * n.x), 
                       int(IntScale * n.y), 
                       int(IntScale * n.z));
    
    // Convert float to int, apply integer offset, then convert back to float
    vec3 p_i = vec3(
        intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0.0) ? -of_i.x : of_i.x)),
        intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0.0) ? -of_i.y : of_i.y)),
        intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0.0) ? -of_i.z : of_i.z))
    );
    
    // Apply appropriate offset based on coordinate magnitude
    return vec3(
        abs(p.x) < OriginScale ? p.x + FloatScale * n.x : p_i.x,
        abs(p.y) < OriginScale ? p.y + FloatScale * n.y : p_i.y,
        abs(p.z) < OriginScale ? p.z + FloatScale * n.z : p_i.z
    );
}

/**
 * @brief Generates a new ray from an intersection point with origin offset to avoid self-intersection
 * @param position Intersection point in world space
 * @param direction Ray direction vector (should be normalized)
 * @param normal Surface normal at the intersection point (should be normalized)
 * @param tmin Minimum ray distance to prevent self-intersection
 * @param tmax Maximum ray distance for intersection testing
 * @return New ray with properly offset origin to prevent numerical precision issues
 */
Ray SpawnRay(vec3 position, vec3 direction, vec3 normal, float tmin, float tmax) {
    vec3 offset_normal = (dot(normal, direction) >= 0.0f) ? normal : -normal;
    vec3 offset_origin = OffsetRayOrigin(position, offset_normal);
    
    Ray ray;
    ray.origin = offset_origin;
    ray.direction = direction;
    ray.tmin = tmin;
    ray.tmax = tmax;
    
    return ray;
}

/**
 * @brief Generates a new ray from an intersection point with origin offset to avoid self-intersection
 * @param position Intersection point in world space
 * @param direction Ray direction vector (should be normalized)
 * @param distance Maximum distance to the target point (typically to light source)
 * @return New ray with properly offset origin to prevent numerical precision issues
 */
Ray SpawnShadowRay(vec3 position, vec3 direction, float distance) {
    Ray ray;
    ray.origin = position;
    ray.direction = direction;
    ray.tmin = Epsilon;
    ray.tmax = distance - Epsilon;
    
    return ray;
}

#endif // UTIL_GLSL