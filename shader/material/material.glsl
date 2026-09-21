#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "material/microfacet.glsl"
#include "sample/sampling.glsl"
#include "sample/sampler.glsl"

uniform sampler2DArray TextureArray;
layout(location = 4) uniform int TextureCount;

// Material type enumeration, consistent with C++ side
const int MaterialType_Boundary = 0;
const int MaterialType_Diffuse = 1;   ///< Diffuse material (Oren-Nayar model)
const int MaterialType_Conductor = 2; ///< Conductor material (GGX Microfacet Model)

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
 * @brief Retrieves the final anisotropic roughness with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @return vec2 containing anisotropic roughness values (x = U direction, y = V direction);
 *         uses texture's R and G channels if roughness_aniso_texture is available,
 *         otherwise falls back to base material roughness_aniso
 */
vec2 GetFinalRoughnessAniso(IntersectionInfo info) {
    if (info.material.roughness_aniso_texture >= 0 && info.material.roughness_aniso_texture < TextureCount) {
        // Sample anisotropic roughness texture (R channel = U, G channel = V)
        vec4 tex_color = SampleTextureArray(info.material.roughness_aniso_texture, info.uv);

        return vec2(tex_color.r, tex_color.g);
    }
    
    // Fallback: Use base anisotropic roughness
    return info.material.roughness_aniso;
}

/**
 * @brief Retrieves the final specular color with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @param lambda Sampled wavelengths for spectral rendering conversion
 * @return SampledSpectrum representing the final specular color; uses texture if available, otherwise falls back to base material specular
 */
SampledSpectrum GetFinalSpecular(IntersectionInfo info, SampledWavelengths lambda) {
    if (info.material.specular_texture >= 0 && info.material.specular_texture < TextureCount) {
        // Sample specular texture
        vec4 tex_color = SampleTextureArray(info.material.specular_texture, info.uv);
        
        // Convert texture RGB to spectrum
        RGB tex_rgb = RGBNew(tex_color.r, tex_color.g, tex_color.b);
        RGBAlbedoSpectrum tex_spectrum = RGBAlbedoSpectrumNew(tex_rgb);

        return RGBAlbedoSpectrumSample(tex_spectrum, lambda);
    }
    
    // Fallback: Use base specular color
    return info.material.specular;
}

/**
 * @brief Retrieves the conductor eta (real part of complex IOR)
 * @note Eta has no texture support, always returns the constant material value
 * @param info Intersection data containing material properties
 * @return SampledSpectrum representing the conductor eta
 */
SampledSpectrum GetFinalEta(IntersectionInfo info) {
    return info.material.eta;
}

/**
 * @brief Retrieves the conductor k (imaginary part of complex IOR)
 * @note K has no texture support, always returns the constant material value
 * @param info Intersection data containing material properties
 * @return SampledSpectrum representing the conductor k
 */
SampledSpectrum GetFinalK(IntersectionInfo info) {
    return info.material.k;
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
 * @brief Evaluates the BSDF and PDF for a conductor material (GGX microfacet)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param world_out Out direction in world space
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo ConductorEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out, SampledWavelengths lambda) {
    MaterialEvalInfo m_info;
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    // Get anisotropic roughness (alpha_u, alpha_v)
    vec2 aniso_roughness = GetFinalRoughnessAniso(info);
    float alpha_u = aniso_roughness.x;
    float alpha_v = aniso_roughness.y;

    SampledSpectrum eta = GetFinalEta(info);
    SampledSpectrum k = GetFinalK(info);
    SampledSpectrum specular = GetFinalSpecular(info, lambda);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);
    vec3 l = normalize(world_out);
    vec3 h = normalize(v + l);

    float n_dot_v = dot(n, v);
    float n_dot_l = dot(n, l);
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f) {
        return m_info;
    }

    // PDF using the visible normal distribution
    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float pdf = Dv * abs(1.0f / (4.0f * dot(v, h)));

    // Microfacet BRDF terms
    SampledSpectrum F = FresnelConductor(v, h, eta, k);
    float G = GGXG1(v, h, n, alpha_u, alpha_v) *
              GGXG1(l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    // BRDF = specular * F * D * G / (4 * NdotV * NdotL)
    // Multiple scattering term intentionally omitted for now
    SampledSpectrum brdf = MulFloat(Mul(specular, F), D * G / (4.0f * n_dot_v * n_dot_l));

    m_info.bsdf = brdf;
    m_info.pdf = pdf;
    return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a conductor material (GGX microfacet)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo ConductorSample(IntersectionInfo info, vec3 world_in, vec2 sample_xy, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    vec2 aniso_roughness = GetFinalRoughnessAniso(info);
    float alpha_u = aniso_roughness.x;
    float alpha_v = aniso_roughness.y;

    SampledSpectrum eta = GetFinalEta(info);
    SampledSpectrum k = GetFinalK(info);
    SampledSpectrum specular = GetFinalSpecular(info, lambda);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);

    // Sample visible normal distribution (returns local-space half vector)
    vec3 local_h = GGXSampleVisible(n, v, alpha_u, alpha_v, sample_xy);
    vec3 h = ToWorldFromUp(local_h, n);

    // Reflect incident direction around sampled half vector
    vec3 l = reflect(-v, h);

    float n_dot_v = dot(n, v);
    float n_dot_l = dot(n, l);
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f) {
        return m_info;
    }

    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float pdf = Dv * abs(1.0f / (4.0f * dot(v, h)));

    SampledSpectrum F = FresnelConductor(v, h, eta, k);
    float G = GGXG1(v, h, n, alpha_u, alpha_v) *
              GGXG1(l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    SampledSpectrum brdf = MulFloat(Mul(specular, F), D * G / (4.0f * n_dot_v * n_dot_l));

    m_info.world_out = l;
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
    else if (MaterialType_Conductor == info.material.type) {
        return ConductorEvaluate(info, world_in, world_out, lambda);
    }
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
    else if (MaterialType_Conductor == info.material.type) {
        return ConductorSample(info, world_in, sample_xy, lambda);
    }
    // else if (info.material.type == MaterialType_Dielectric) {
    //     return DielectricSample(info, world_in, sample_xy, lambda);
    // }
    
    // Unknown material type, return zero contribution
    return result;
}

#endif // MATERIAL_GLSL