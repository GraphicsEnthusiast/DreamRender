#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "material/microfacet.glsl"
#include "sample/sampling.glsl"
#include "sample/sampler.glsl"

uniform sampler2DArray TextureArray;
layout(location = 4) uniform int TextureCount;

// Material type enumeration, consistent with C++ side
const int MaterialType_Boundary = 0;
const int MaterialType_Diffuse = 1;  ///< Diffuse material (Oren-Nayar model)

/**
 * @struct MaterialEvalInfo
 * @brief Stores the result of material evaluation during light transport simulation.
 */
struct MaterialEvalInfo {
    SampledSpectrum bsdf;
    float pdf;
};

/**
 * @struct MaterialSampleInfo
 * @brief Stores the result of sampling a direction during light transport simulation.
 */
struct MaterialSampleInfo {
    vec3 world_out;
    SampledSpectrum bsdf;
    float pdf;
};

/**
 * @brief Validates texture index to ensure it's within the valid range
 * @param tex_id Texture index to be checked
 * @return Boolean indicating if the texture index is valid (true if tex_id is between 0 and TextureCount-1 inclusive)
 */
bool IsTextureValid(int tex_id) {
    return tex_id >= 0 && tex_id < TextureCount;
}

/**
 * @brief Samples a texel from the 2D texture array with safety checks
 * @param tex_id Index of the texture layer in the texture array
 * @param uv 2D texture coordinates in normalized [0,1] range
 * @return Sampled RGBA color value; returns bright red (1.0, 0.0, 0.0, 1.0) for invalid texture IDs
 */
vec4 SampleTextureArray(int tex_id, vec2 uv) {
    if (!IsTextureValid(tex_id)) {
        return vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    return texture(TextureArray, vec3(uv, float(tex_id)));
}

/**
 * @brief Retrieves the final diffuse color with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @param lambda Sampled wavelengths for spectral rendering conversion
 * @return SampledSpectrum representing the final diffuse color; uses texture if available, otherwise falls back to base material diffuse
 */
SampledSpectrum GetFinalDiffuse(IntersectionInfo info, SampledWavelengths lambda) {
    if (info.material.diffuse_texture >= 0 && info.material.diffuse_texture < TextureCount) {
        // Sample texture
        vec4 tex_color = SampleTextureArray(info.material.diffuse_texture, info.uv);
        
        // Convert texture RGB to spectrum
        RGB tex_rgb = RGBNew(tex_color.r, tex_color.g, tex_color.b);
        RGBAlbedoSpectrum tex_spectrum = RGBAlbedoSpectrumNew(tex_rgb);

        return RGBAlbedoSpectrumSample(tex_spectrum, lambda);
    }
    
    // Fallback: Use base diffuse color
    return info.material.diffuse;
}

/**
 * @brief Retrieves the final roughness value with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @return Final roughness value; uses texture's red channel if roughness texture is available, otherwise falls back to base material roughness
 */
float GetFinalRoughness(IntersectionInfo info) {
    if (info.material.roughness_texture >= 0 && info.material.roughness_texture < TextureCount) {
        // Sample roughness texture (usually in red channel)
        vec4 tex_color = SampleTextureArray(info.material.roughness_texture, info.uv);

        return tex_color.r;
    }
    
    // Fallback: Use base roughness
    return info.material.roughness;
}

/**
 * @brief Evaluates the BSDF and PDF for a diffuse material(Oren-Nayar diffuse model calculation)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param world_out Out direction in world space
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo DiffuseEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out, SampledWavelengths lambda) {
    MaterialEvalInfo m_info;
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = GetFinalDiffuse(info, lambda);
    float roughness = GetFinalRoughness(info);

	vec3 v = normalize(world_in);
	vec3 l = normalize(world_out);
	vec3 n = normalize(info.shading_normal);
	vec3 h = normalize(v + l);
	float n_dot_l = dot(n, l);
	float n_dot_v = dot(n, v);
	float h_dot_v = dot(h, v);

	if (n_dot_l <= 0.0f || n_dot_v <= 0.0f) {
		return m_info;
	}

	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float v_dot_l = 2.0f * h_dot_v * h_dot_v - 1.0f;
	float cosri = v_dot_l - n_dot_v * n_dot_l;
	float c1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
	float c2 = 0.45f * s2 / (s2 + 0.09f) * cosri * (cosri >= 0.0f ? (max(n_dot_l, n_dot_v)) : 1.0f);

    // diffuse brdf: (diffuse / π) * (c1 + c2) * (1 + roughness * 0.5)
	SampledSpectrum brdf = MulFloat(diffuse, (1.0f / PI) * (c1 + c2) * (1.0f + roughness * 0.5f)) ;
	float pdf = CosineHemispherePDF(n_dot_l);

    m_info.bsdf = brdf;
    m_info.pdf = pdf;

	return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a diffuse material(Oren-Nayar diffuse model calculation)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo DiffuseSample(IntersectionInfo info, vec3 world_in, vec2 sample_xy, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = GetFinalDiffuse(info, lambda);
    float roughness = GetFinalRoughness(info);

    vec3 n = normalize(info.shading_normal);
    vec3 local_l = CosineHemisphereSample(sample_xy);
    vec3 world_out = ToWorldFromUp(local_l, n);
    vec3 v = normalize(world_in);
    vec3 l = normalize(world_out);
    vec3 h = normalize(v + l);

    float n_dot_l = dot(n, l);
    float n_dot_v = dot(n, v);
    float h_dot_v = dot(h, v);

    if (n_dot_l <= 0.0f || n_dot_v <= 0.0f) {
        return m_info;
    }

    float a = roughness * roughness;
    float s = a;
    float s2 = s * s;
    float v_dot_l = 2.0f * h_dot_v * h_dot_v - 1.0f;
    float cosri = v_dot_l - n_dot_v * n_dot_l;
    
    float c1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
    float c2 = 0.45f * s2 / (s2 + 0.09f) * cosri * (cosri >= 0.0f ? max(n_dot_l, n_dot_v) : 1.0f);

    SampledSpectrum brdf = MulFloat(diffuse, (1.0f / PI) * (c1 + c2) * (1.0f + roughness * 0.5f));
    float pdf = CosineHemispherePDF(n_dot_l);

    m_info.world_out = world_out;
    m_info.bsdf = brdf;
    m_info.pdf = pdf;

    return m_info;
}

/**
 * @brief Unified material evaluation function that dispatches to the appropriate material model
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param world_out Out direction in world space
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo MaterialEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out, SampledWavelengths lambda) {
    MaterialEvalInfo result;
    result.bsdf = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on material type
    if (MaterialType_Diffuse == info.material.type) {
        return DiffuseEvaluate(info, world_in, world_out, lambda);
    }
    
    // Add more material types here in the future
    // else if (info.material.type == MaterialType_Metal) {
    //     return MetalEvaluate(info, world_in, world_out, lambda);
    // }
    // else if (info.material.type == MaterialType_Dielectric) {
    //     return DielectricEvaluate(info, world_in, world_out, lambda);
    // }
    
    // Unknown material type, return zero contribution
    return result;
}

/**
 * @brief Unified material sampling function that dispatches to the appropriate material model
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo MaterialSample(IntersectionInfo info, vec3 world_in, vec2 sample_xy, SampledWavelengths lambda) {
    MaterialSampleInfo result;
    result.world_out = vec3(0.0f);
    result.bsdf = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on material type
    if (MaterialType_Diffuse == info.material.type) {
        return DiffuseSample(info, world_in, sample_xy, lambda);
    }
    
    // Add more material types here in the future
    // else if (info.material.type == MaterialType_Metal) {
    //     return MetalSample(info, world_in, sample_xy, lambda);
    // }
    // else if (info.material.type == MaterialType_Dielectric) {
    //     return DielectricSample(info, world_in, sample_xy, lambda);
    // }
    
    // Unknown material type, return zero contribution
    return result;
}

#endif // MATERIAL_GLSL