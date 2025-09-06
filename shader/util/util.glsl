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
