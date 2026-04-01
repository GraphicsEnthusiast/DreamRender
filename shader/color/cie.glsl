#ifndef CIE_GLSL
#define CIE_GLSL

#include "util/util.glsl"

const int CIE_OFFSET_X = 0;
const int CIE_OFFSET_Y = NCIESamples;
const int CIE_OFFSET_Z = 2 * NCIESamples;
const int CIE_OFFSET_D65 = 3 * NCIESamples;

layout(std430, binding = 8) readonly buffer CIETable {
    float CIEData[];
};

float CIEX(int index) {
    return CIEData[CIE_OFFSET_X + index];
}

float CIEY(int index) {
    return CIEData[CIE_OFFSET_Y + index];
}

float CIEZ(int index) {
    return CIEData[CIE_OFFSET_Z + index];
}

float D65(int index) {
    return CIEData[CIE_OFFSET_D65 + index];
}

// Predefined RGB to XYZ conversion matrix (D65 white point)
const mat3 RGBTOXYZ = mat3(
    0.412453, 0.357580, 0.180423,
    0.212671, 0.715160, 0.072169,
    0.019334, 0.119193, 0.950227
);

// Predefined XYZ to RGB conversion matrix (D65 white point)
const mat3 XYZTORGB = mat3(
     3.240479, -1.537150, -0.498535,
    -0.969256,  1.875991,  0.041556,
     0.055648, -0.204043,  1.057311
);

#endif // CIE_GLSL