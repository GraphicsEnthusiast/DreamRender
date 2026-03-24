#ifndef _MATERIAL__GLSL__
#define _MATERIAL__GLSL__

#include "spectrum/spectrum.glsl"
#include "sample/sampling.glsl"
#include "sample/sampler.glsl"

uniform sampler2DArray TextureArray;
uniform int TextureCount;

// Material type enumeration, consistent with C++ side
const int MaterialType_Diffuse = 0;  ///< Diffuse material (Oren-Nayar model)

/**
 * @struct MaterialEvalInfo
 * @brief Stores the result of material evaluation during light transport simulation.
 */
struct MaterialEvalInfo {
    SampledSpectrum bsdf_cosine;
    float pdf;
};

/**
 * @struct MaterialSampleInfo
 * @brief Stores the result of sampling a direction during light transport simulation.
 */
struct MaterialSampleInfo {
    vec3 world_l;
    SampledSpectrum bsdf_cosine;
    float pdf;
};

/**
 * @brief Checks if texture ID is valid
 */
bool IsTextureValid(int tex_id) {
    return tex_id >= 0 && tex_id < TextureCount;
}

/**
 * @brief Samples a texture from the texture array
 */
vec4 SampleTextureArray(int tex_id, vec2 uv) {
    if (!IsTextureValid(tex_id)) {
        return vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    return texture(TextureArray, vec3(uv, float(tex_id)));
}

/**
 * @brief Gets the final diffuse color with texture support
 */
SampledSpectrum GetFinalDiffuse(IntersectionInfo info, vec2 uv, SampledWavelengths lambda) {
    if (info.material.diffuse_texture >= 0 && info.material.diffuse_texture < TextureCount) {
        // Sample texture
        vec4 tex_color = SampleTextureArray(info.material.diffuse_texture, uv);
        
        // Convert texture RGB to spectrum
        RGB tex_rgb = RGBNew(tex_color.r, tex_color.g, tex_color.b);
        RGBAlbedoSpectrum tex_spectrum = RGBAlbedoSpectrumNew(tex_rgb);

        return RGBAlbedoSpectrumSample(tex_spectrum, lambda);
    }
    
    // Fallback: Use base diffuse color
    return info.material.diffuse;
}

/**
 * @brief Gets the final roughness with texture support
 */
float GetFinalRoughness(IntersectionInfo info, vec2 uv) {
    if (info.material.roughness_texture >= 0 && info.material.roughness_texture < TextureCount) {
        // Sample roughness texture (usually in red channel)
        vec4 tex_color = SampleTextureArray(info.material.roughness_texture, uv);

        return tex_color.r;
    }
    
    // Fallback: Use base roughness
    return info.material.roughness;
}

/**
 * @brief Evaluates the BSDF and PDF for a diffuse material(Oren-Nayar diffuse model calculation)
 * @param info Intersection data containing material properties and surface normal
 * @param world_v View direction in world space (pointing toward camera)
 * @param world_l Light direction in world space (pointing toward light source)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo DiffuseEvaluate(IntersectionInfo info, vec3 world_v, vec3 world_l, SampledWavelengths lambda) {
    MaterialEvalInfo m_info;
    m_info.bsdf_cosine = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = GetFinalDiffuse(info, info.uv, lambda);
    float roughness = GetFinalRoughness(info, info.uv);

	vec3 v = normalize(world_v);
	vec3 l = normalize(world_l);
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

    m_info.bsdf_cosine = MulFloat(brdf, n_dot_l);
    m_info.pdf = pdf;

	return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a diffuse material(Oren-Nayar diffuse model calculation)
 * @param info Intersection data containing material properties and surface normal
 * @param world_v View direction in world space (pointing toward camera)
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo DiffuseSample(IntersectionInfo info, vec3 world_v, vec2 sample_xy, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_l = vec3(0.0f);
    m_info.bsdf_cosine = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = GetFinalDiffuse(info, info.uv, lambda);
    float roughness = GetFinalRoughness(info, info.uv);

    vec3 n = normalize(info.shading_normal);
    vec3 local_l = CosineHemisphereSample(sample_xy);
    vec3 world_l = ToWorldFromUp(local_l, n);
    vec3 v = normalize(world_v);
    vec3 l = normalize(world_l);
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

    m_info.world_l = world_l;
    m_info.bsdf_cosine = MulFloat(brdf, n_dot_l);
    m_info.pdf = pdf;

    return m_info;
}

/**
 * @brief Unified material evaluation function that dispatches to the appropriate material model
 * @param info Intersection data containing material properties and surface normal
 * @param world_v View direction in world space (pointing toward camera)
 * @param world_l Light direction in world space (pointing toward light source)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo MaterialEvaluate(IntersectionInfo info, vec3 world_v, vec3 world_l, SampledWavelengths lambda) {
    MaterialEvalInfo result;
    result.bsdf_cosine = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on material type
    if (MaterialType_Diffuse == info.material.type) {
        return DiffuseEvaluate(info, world_v, world_l, lambda);
    }
    
    // Add more material types here in the future
    // else if (info.material.type == MaterialType_Metal) {
    //     return MetalEvaluate(info, world_v, world_l, lambda);
    // }
    // else if (info.material.type == MaterialType_Dielectric) {
    //     return DielectricEvaluate(info, world_v, world_l, lambda);
    // }
    
    // Unknown material type, return zero contribution
    return result;
}

/**
 * @brief Unified material sampling function that dispatches to the appropriate material model
 * @param info Intersection data containing material properties and surface normal
 * @param world_v View direction in world space (pointing toward camera)
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo MaterialSample(IntersectionInfo info, vec3 world_v, vec2 sample_xy, SampledWavelengths lambda) {
    MaterialSampleInfo result;
    result.world_l = vec3(0.0f);
    result.bsdf_cosine = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on material type
    if (MaterialType_Diffuse == info.material.type) {
        return DiffuseSample(info, world_v, sample_xy, lambda);
    }
    
    // Add more material types here in the future
    // else if (info.material.type == MaterialType_Metal) {
    //     return MetalSample(info, world_v, sample_xy, lambda);
    // }
    // else if (info.material.type == MaterialType_Dielectric) {
    //     return DielectricSample(info, world_v, sample_xy, lambda);
    // }
    
    // Unknown material type, return zero contribution
    return result;
}

#endif // _MATERIAL__GLSL__