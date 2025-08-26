#ifndef _COLOR__GLSL__
#define _COLOR__GLSL__

#include "color/cie.glsl"

// RGB Definition
struct RGB {
    float r;
    float g;
    float b;
};

/**
 * @brief Construct RGB with values
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @return New RGB struct
 */
RGB RGBNew(float r, float g, float b) {
    RGB color;
    color.r = r;
    color.g = g;
    color.b = b;

    return color;
}

/**
 * @brief Add two RGB colors component-wise
 * @param a First color
 * @param b Second color
 * @return Component-wise sum
 */
RGB RGBAdd(RGB a, RGB b) {
    RGB result;
    result.r = a.r + b.r;
    result.g = a.g + b.g;
    result.b = a.b + b.b;

    return result;
}

/**
 * @brief Subtract two RGB colors component-wise
 * @param a Minuend color
 * @param b Subtrahend color
 * @return Component-wise difference
 */
RGB RGBSub(RGB a, RGB b) {
    RGB result;
    result.r = a.r - b.r;
    result.g = a.g - b.g;
    result.b = a.b - b.b;

    return result;
}

/**
 * @brief Multiply two RGB colors component-wise
 * @param a First color
 * @param b Second color
 * @return Component-wise product
 */
RGB RGBMul(RGB a, RGB b) {
    RGB result;
    result.r = a.r * b.r;
    result.g = a.g * b.g;
    result.b = a.b * b.b;

    return result;
}

/**
 * @brief Scale RGB color by scalar
 * @param s Input color
 * @param a Scalar multiplier
 * @return Scaled color
 */
RGB RGBMulFloat(RGB s, float a) {
    RGB result;
    result.r = s.r * a;
    result.g = s.g * a;
    result.b = s.b * a;

    return result;
}

/**
 * @brief Divide RGB color by scalar
 * @param s Input color
 * @param a Scalar divisor
 * @return Divided color
 */
RGB RGBDivFloat(RGB s, float a) {
    RGB result;
    result.r = s.r / a;
    result.g = s.g / a;
    result.b = s.b / a;

    return result;
}

/**
 * @brief Negate RGB color
 * @param s Input color
 * @return Negated color
 */
RGB RGBNegate(RGB s) {
    RGB result;
    result.r = -s.r;
    result.g = -s.g;
    result.b = -s.b;

    return result;
}

/**
 * @brief Compute average of RGB components
 * @param s Input color
 * @return Average value
 */
float RGBAverage(RGB s) {
    return (s.r + s.g + s.b) / 3.0f;
}

/**
 * @brief Component-wise maximum of two RGB colors
 * @param a First color
 * @param b Second color
 * @return Component-wise max
 */
RGB RGBMax(RGB a, RGB b) {
    RGB result;
    result.r = max(a.r, b.r);
    result.g = max(a.g, b.g);
    result.b = max(a.b, b.b);

    return result;
}

/**
 * @brief Linear interpolation between two RGB colors
 * @param t Interpolation factor [0,1]
 * @param s1 Start color
 * @param s2 End color
 * @return Interpolated color
 */
RGB RGBLerp(float t, RGB s1, RGB s2) {
    return RGBAdd(RGBMulFloat(s1, 1.0 - t), RGBMulFloat(s2, t));
}

/**
 * @brief Clamp RGB components between min and max
 * @param rgb Input color
 * @param min Lower bound
 * @param max Upper bound
 * @return Clamped color
 */
RGB RGBClamp(RGB rgb, float min, float max) {
    RGB result;
    result.r = clamp(rgb.r, min, max);
    result.g = clamp(rgb.g, min, max);
    result.b = clamp(rgb.b, min, max);

    return result;
}

/**
 * @brief Clamp RGB components to non-negative values
 * @param rgb Input color
 * @return Non-negative color
 */
RGB RGBClampZero(RGB rgb) {
    RGB result;
    result.r = max(rgb.r, 0.0f);
    result.g = max(rgb.g, 0.0f);
    result.b = max(rgb.b, 0.0f);

    return result;
}

// XYZ Definition
struct XYZ {
    float x;
    float y;
    float z;
};

/**
 * @brief Construct XYZ with values
 * @param x x component
 * @param y y component
 * @param z z component
 * @return New XYZ struct
 */
XYZ XYZNew(float x, float y, float z) {
    XYZ color;
    color.x = x;
    color.y = y;
    color.z = z;

    return color;
}

/**
 * @brief Compute average of XYZ components
 * @param s Input color
 * @return Average value
 */
float XYZAverage(XYZ s) {
    return (s.x + s.y + s.z) / 3.0f;
}

/**
 * @brief Convert XYZ to xy chromaticity coordinates
 * @param s Input color
 * @return xy chromaticity
 */
vec2 XYZxy(XYZ s) {
    float sum = s.x + s.y + s.z;

    return vec2(s.x / sum, s.y / sum);
}

/**
 * @brief Convert xyY to XYZ color
 * @param xy Chromaticity coordinates
 * @param y Luminance
 * @return XYZ color
 */
XYZ XYZFromxyY(vec2 xy, float y) {
    if (0.0f == xy.y) {
        return XYZNew(0.0f, 0.0f, 0.0f);
    }

    return XYZNew(
        xy.x * y / xy.y,
        y,
        (1.0f - xy.x - xy.y) * y / xy.y
    );
}

/**
 * @brief Add two XYZ colors component-wise
 * @param a First color
 * @param b Second color
 * @return Component-wise sum
 */
XYZ XYZAdd(XYZ a, XYZ b) {
    XYZ result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

/**
 * @brief Subtract two XYZ colors component-wise
 * @param a Minuend color
 * @param b Subtrahend color
 * @return Component-wise difference
 */
XYZ XYZSub(XYZ a, XYZ b) {
    XYZ result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

/**
 * @brief Multiply two XYZ colors component-wise
 * @param a First color
 * @param b Second color
 * @return Component-wise product
 */
XYZ XYZMul(XYZ a, XYZ b) {
    XYZ result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return result;
}

/**
 * @brief Scale XYZ color by scalar
 * @param s Input color
 * @param a Scalar multiplier
 * @return Scaled color
 */
XYZ XYZMulFloat(XYZ s, float a) {
    XYZ result;
    result.x = s.x * a;
    result.y = s.y * a;
    result.z = s.z * a;

    return result;
}

/**
 * @brief Divide XYZ color by scalar
 * @param s Input color
 * @param a Scalar divisor
 * @return Divided color
 */
XYZ XYZDivFloat(XYZ s, float a) {
    XYZ result;
    result.x = s.x / a;
    result.y = s.y / a;
    result.z = s.z / a;

    return result;
}

/**
 * @brief Negate XYZ color
 * @param s Input color
 * @return Negated color
 */
XYZ XYZNegate(XYZ s) {
    XYZ result;
    result.x = -s.x;
    result.y = -s.y;
    result.z = -s.z;

    return result;
}

/**
 * @brief Linear interpolation between two XYZ colors
 * @param t Interpolation factor [0,1]
 * @param s1 Start color
 * @param s2 End color
 * @return Interpolated color
 */
XYZ XYZLerp(float t, XYZ s1, XYZ s2) {
    return XYZAdd(XYZMulFloat(s1, 1.0f - t), XYZMulFloat(s2, t));
}

/**
 * @brief Clamp XYZ components between min and max
 * @param xyz Input color
 * @param min Lower bound
 * @param max Upper bound
 * @return Clamped color
 */
XYZ XYZClamp(XYZ xyz, float min, float max) {
    XYZ result;
    result.x = clamp(xyz.x, min, max);
    result.y = clamp(xyz.y, min, max);
    result.z = clamp(xyz.z, min, max);

    return result;
}

/**
 * @brief Clamp XYZ components to non-negative values
 * @param xyz Input color
 * @return Non-negative color
 */
XYZ XYZClampZero(XYZ xyz) {
    XYZ result;
    result.x = max(xyz.x, 0.0f);
    result.y = max(xyz.y, 0.0f);
    result.z = max(xyz.z, 0.0f);

    return result;
}

/**
 * @brief Convert XYZ color to RGB (SRGB color gamut coordinates and D65 white point)
 * @param xyz Input XYZ color
 * @return RGB color
 */
RGB XYZToRGB(XYZ xyz) {
    RGB rgb;
    
    // Apply XYZ to RGB conversion matrix
    rgb.r = 3.240479f * xyz.x - 1.537150f * xyz.y - 0.498535f * xyz.z;
    rgb.g = -0.969256f * xyz.x + 1.875991f * xyz.y + 0.041556f * xyz.z;
    rgb.b = 0.055648f * xyz.x - 0.204043f * xyz.y + 1.057311f * xyz.z;
    
    return rgb;
}

#endif