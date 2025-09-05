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
 * @brief Converts degrees to radians
 * @param degrees Angle in degrees
 * @return Angle in radians
 */
float DegreesToRadians(float degrees) {
    return degrees * (PI / 180.0f);
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

#endif // _UTIL__GLSL__
