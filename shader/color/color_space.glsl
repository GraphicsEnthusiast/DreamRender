#ifndef _COLOR_SPACE_GLSL_
#define _COLOR_SPACE_GLSL_

#include "color/color.glsl"

uniform samplerBuffer SRGBToSpectrumTable;

/**
 * @class RGBSigmoidPolynomial
 * @brief Represents a sigmoid polynomial for RGB spectrum modeling(f(λ) = s(c₀λ² + c₁λ + c₂), Coefficients storage (c₀, c₁, c₂ for λ², λ, constant terms))
 */
struct RGBSigmoidPolynomial {
    float c0;
    float c1;
    float c2;
};

/**
 * @brief Evaluate polynomial at given wavelength
 * @param poly Polynomial coefficients
 * @param lambda Wavelength in nanometers (typically 360-830nm)
 * @return Normalized sigmoid output in [0,1] range
 */
float RGBSigmoidPolynomialEval(RGBSigmoidPolynomial poly, float lambda) {
    float poly_val = poly.c0 * lambda * lambda + poly.c1 * lambda + poly.c2;
    
    // Sigmoid normalization: s(x) = 0.5 + x/(2√(1+x²))
    if (isinf(poly_val)) {
        return poly_val > 0.0f ? 1.0f : 0.0f;
    }
    float denom = 2.0f * sqrt(1.0f + poly_val * poly_val);
    
    return 0.5f + poly_val / denom;
}

/**
 * @brief Compute maximum value in visible spectrum (360-830nm)
 * @param poly Polynomial coefficients
 * @return Peak response value within valid wavelength range
 */
float RGBSigmoidPolynomialMaxValue(RGBSigmoidPolynomial poly) {
    // Check spectrum boundaries
    float max_val = max(RGBSigmoidPolynomialEval(poly, LambdaMin), 
                       RGBSigmoidPolynomialEval(poly, LambdaMax));
    
    // Compute vertex position: λ = -c₁/(2c₀)
    if (0.0f != poly.c0) {
        float vertex_lambda = -poly.c1 / (2.0f * poly.c0);
        if (vertex_lambda >= LambdaMin && vertex_lambda <= LambdaMax) {
            max_val = max(max_val, RGBSigmoidPolynomialEval(poly, vertex_lambda));
        }
    }
    
    return max_val;
}

/**
 * @brief Converts CIE XYZ values to RGB color space
 * @param xyz Input XYZ color values
 * @return RGB color values in sRGB color space
 */
RGB XYZToRGB(XYZ xyz) {
    vec3 rgb_vec = XYZTORGB * vec3(xyz.x, xyz.y, xyz.z);

    return RGBNew(rgb_vec.r, rgb_vec.g, rgb_vec.b);
}

/**
 * @brief Converts RGB values to CIE XYZ color space
 * @param rgb Input RGB color values
 * @return XYZ color values
 */
XYZ RGBToXYZ(RGB rgb) {
    vec3 xyz_vec = RGBTOXYZ * vec3(rgb.r, rgb.g, rgb.b);

    return XYZNew(xyz_vec.x, xyz_vec.y, xyz_vec.z);
}

/**
 * @brief Samples the D65 illuminant spectrum at a given wavelength
 * @param lambda Wavelength in nanometers
 * @return Spectral radiance at the given wavelength
 */
float SampleD65Illuminant(float lambda) {
    int offset = int(round(lambda) - int(LambdaMin));
	if (offset < 0 || offset >= NCIESamples) {
		return 0.0f;
	}

	return D65[offset];
}

const int RGBToSpectrumTableRes = 64;

/**
 * @brief Computes the inverse of the smooth step function
 * @param x Input value in the range [0, 1]
 * @return The inverse smooth step value, transforming the smoothed input back towards its original domain
 */
float InverseSmoothStep(float x) {
    return 0.5f - sin(asin(1.0f - 2.0f * x) * (1.0f / 3.0f));
}

/**
 * @brief Computes the flattened index into the SRGBToSpectrumTableData array
 * @param maxc The maximum color channel index (0, 1, or 2 for R, G, B respectively)
 * @param z The coordinate in the first spatial dimension [0, RGBToSpectrumTableRes-1]
 * @param y The coordinate in the second spatial dimension [0, RGBToSpectrumTableRes-1]
 * @param x The coordinate in the third spatial dimension [0, RGBToSpectrumTableRes-1]
 * @param i The coefficient index within the spectral polynomial [0, 3]
 * @return Linear index into the flattened SRGBToSpectrumTableData array
 */
int GetSpectrumTableIndex(int maxc, int z, int y, int x, int i) {
    return (((maxc * RGBToSpectrumTableRes + z) * RGBToSpectrumTableRes + y) * RGBToSpectrumTableRes + x) * 4 + i;
}

/**
 * @brief Convert RGB color to spectral coefficients
 * @param rgb Input RGB color (components should be in [0,1] range)
 * @return RGBSigmoidPolynomial coefficients for spectral representation
 */
RGBSigmoidPolynomial RGBToSpectrumTableEval(RGB rgb) {
    // Clamp RGB values to valid [0, 1] range
    rgb.r = clamp(rgb.r, 0.0f, 1.0f);
    rgb.g = clamp(rgb.g, 0.0f, 1.0f);
    rgb.b = clamp(rgb.b, 0.0f, 1.0f);

    // Handle uniform RGB values (grayscale)
    if (rgb.r == rgb.g && rgb.g == rgb.b) {
        float value = rgb.r;
        float c2 = (value - 0.5f) / sqrt(value * (1.0f - value));

        return RGBSigmoidPolynomial(0.0f, 0.0f, c2);
    }

    // Find maximum RGB component
    int maxc;
    if (rgb.r > rgb.g) {
        maxc = (rgb.r > rgb.b) ? 0 : 2;
    } 
    else {
        maxc = (rgb.g > rgb.b) ? 1 : 2;
    }
    
    float z_val = 0.0f;
    if (0 == maxc) {
        z_val = rgb.r;
    }
    else if (maxc == 1) {
        z_val = rgb.g;
    }
    else {
        z_val = rgb.b;
    }
    
    // Compute remapped component values
    float x = GetRGBComponent(rgb, (maxc + 1) % 3) * (float(RGBToSpectrumTableRes) - 1.0f) / z_val;
    float y = GetRGBComponent(rgb, (maxc + 2) % 3) * (float(RGBToSpectrumTableRes) - 1.0f) / z_val;
    
    // Apply inverse smooth step transformation
    float zz = InverseSmoothStep(InverseSmoothStep(z_val)) * (float(RGBToSpectrumTableRes) - 1.0f);
    
    // Compute integer indices
    int xi = min(int(x), RGBToSpectrumTableRes - 2);
    int yi = min(int(y), RGBToSpectrumTableRes - 2);
    int zi = min(int(zz), RGBToSpectrumTableRes - 2);
    
    // Compute fractional offsets
    float dx = x - float(xi);
    float dy = y - float(yi);
    float dz = zz - float(zi);
    
    // Trilinearly interpolate coefficients
    float c[3] = { 0.0f, 0.0f, 0.0f };
    
    for (int i = 0; i < 3; ++i) {
        int idx000 = GetSpectrumTableIndex(maxc, zi, yi, xi, i);
        int idx100 = GetSpectrumTableIndex(maxc, zi, yi, xi + 1, i);
        int idx010 = GetSpectrumTableIndex(maxc, zi, yi + 1, xi, i);
        int idx110 = GetSpectrumTableIndex(maxc, zi, yi + 1, xi + 1, i);
        int idx001 = GetSpectrumTableIndex(maxc, zi + 1, yi, xi, i);
        int idx101 = GetSpectrumTableIndex(maxc, zi + 1, yi, xi + 1, i);
        int idx011 = GetSpectrumTableIndex(maxc, zi + 1, yi + 1, xi, i);
        int idx111 = GetSpectrumTableIndex(maxc, zi + 1, yi + 1, xi + 1, i);
        
        float val000 = texelFetch(SRGBToSpectrumTable, idx000).r;
        float val100 = texelFetch(SRGBToSpectrumTable, idx100).r;
        float val010 = texelFetch(SRGBToSpectrumTable, idx010).r;
        float val110 = texelFetch(SRGBToSpectrumTable, idx110).r;
        float val001 = texelFetch(SRGBToSpectrumTable, idx001).r;
        float val101 = texelFetch(SRGBToSpectrumTable, idx101).r;
        float val011 = texelFetch(SRGBToSpectrumTable, idx011).r;
        float val111 = texelFetch(SRGBToSpectrumTable, idx111).r;
        
        // Trilinear interpolation
        float c00 = mix(val000, val100, dx);
        float c10 = mix(val010, val110, dx);
        float c0 = mix(c00, c10, dy);
        
        float c01 = mix(val001, val101, dx);
        float c11 = mix(val011, val111, dx);
        float c1 = mix(c01, c11, dy);
        
        c[i] = mix(c0, c1, dz);
    }
    
    return RGBSigmoidPolynomial(c[0], c[1], c[2]);
}

/**
 * @brief Converts RGB values to spectral coefficients using sigmoid polynomials
 * @param rgb Input RGB color values
 * @return RGBSigmoidPolynomial coefficients for spectral representation
 */
RGBSigmoidPolynomial ToRGBCoeffs(RGB rgb) {
    return RGBToSpectrumTableEval(rgb);
}

#endif // _COLOR_SPACE_GLSL_