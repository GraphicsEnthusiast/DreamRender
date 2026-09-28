#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "material/microfacet.glsl"
#include "sample/sampling.glsl"
#include "sample/sampler.glsl"

uniform sampler2DArray TextureArray;
uniform int TextureCount;

// Material type enumeration, consistent with C++ side
const int MaterialType_Boundary = 0;
const int MaterialType_Diffuse = 1;          ///< Diffuse material (Oren-Nayar model)
const int MaterialType_Conductor = 2;        ///< Conductor material (GGX Microfacet Model)
const int MaterialType_Dielectric = 3;       ///< Dielectric material (GGX Microfacet Model)
const int MaterialType_Plastic = 4;          ///< Plastic material (GGX Microfacet Model)
const int MaterialType_MetalWorkflow = 5; ///< Metallic workflow material (GGX Microfacet Model)

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
 * @brief Retrieves the final metallic with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @return float containing metallic value
 */
float GetFinalMetallic(IntersectionInfo info) {
    // Fallback: Use base metallic
    float result = info.material.metallic;

    int tex_u = info.material.metallic_texture;

    if (tex_u >= 0 && tex_u < TextureCount) {
        vec4 tex_color = SampleTextureArray(tex_u, info.uv);
        result = tex_color.r;
    }

    return result;
}

/**
 * @brief Retrieves the final anisotropic roughness U with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @return float containing anisotropic roughness U value
 */
float GetFinalRoughnessU(IntersectionInfo info) {
    // Fallback: Use base anisotropic roughness U
    float result = info.material.roughness_u;

    int tex_u = info.material.roughness_aniso_texture_u;

    if (tex_u >= 0 && tex_u < TextureCount) {
        vec4 tex_color = SampleTextureArray(tex_u, info.uv);
        result = tex_color.r;
    }

    return result;
}

/**
 * @brief Retrieves the final anisotropic roughness V with texture mapping support
 * @param info Intersection data containing material properties and UV coordinates
 * @return float containing anisotropic roughness V value
 */
float GetFinalRoughnessV(IntersectionInfo info) {
    // Fallback: Use base anisotropic roughness V
    float result = info.material.roughness_v;

    int tex_v = info.material.roughness_aniso_texture_v;

    if (tex_v >= 0 && tex_v < TextureCount) {
        vec4 tex_color = SampleTextureArray(tex_v, info.uv);
        result = tex_color.r;
    }

    return result;
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
 * @param info Intersection data containing material properties
 * @return SampledSpectrum representing the conductor eta
 */
SampledSpectrum GetFinalEta(IntersectionInfo info) {
    return info.material.eta;
}

/**
 * @brief Retrieves the conductor k (imaginary part of complex IOR)
 * @param info Intersection data containing material properties
 * @return SampledSpectrum representing the conductor k
 */
SampledSpectrum GetFinalK(IntersectionInfo info) {
    return info.material.k;
}

/**
 * @brief Retrieves the internal medium's index of refraction (IOR)
 * @param info Intersection data containing material properties
 * @return Float representing the internal IOR (e.g., 1.5f for glass)
 */
float GetInIOR(IntersectionInfo info) {
    return info.material.in_ior;
}

/**
 * @brief Retrieves the external medium's index of refraction (IOR)
 * @param info Intersection data containing material properties
 * @return Float representing the external IOR (e.g., 1.0f for vacuum/air)
 */
float GetOutIOR(IntersectionInfo info) {
    return info.material.out_ior;
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
    float roughness = GetFinalRoughnessU(info);

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
 * @param sobol_sampler Sobol sequence sampler (consumes 2 samples)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo DiffuseSample(IntersectionInfo info, vec3 world_in, inout SobolSampler sobol_sampler, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = GetFinalDiffuse(info, lambda);
    float roughness = GetFinalRoughnessU(info);

    vec3 n = normalize(info.shading_normal);
    vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
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

    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

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
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    // BRDF = specular * F * D * G / (4 * NdotV * NdotL)
    SampledSpectrum brdf = MulFloat(Mul(specular, F), D * G / (4.0f * n_dot_v * n_dot_l));

    m_info.bsdf = brdf;
    m_info.pdf = pdf;

    return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a conductor material (GGX microfacet)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sobol_sampler Sobol sequence sampler (consumes 2 samples)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo ConductorSample(IntersectionInfo info, vec3 world_in, inout SobolSampler sobol_sampler, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    SampledSpectrum eta = GetFinalEta(info);
    SampledSpectrum k = GetFinalK(info);
    SampledSpectrum specular = GetFinalSpecular(info, lambda);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);

    // Sample visible normal distribution (returns local-space half vector)
    vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
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
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    SampledSpectrum brdf = MulFloat(Mul(specular, F), D * G / (4.0f * n_dot_v * n_dot_l));

    m_info.world_out = l;
    m_info.bsdf = brdf;
    m_info.pdf = pdf;

    return m_info;
}

/**
 * @brief Evaluates the BSDF and PDF for a dielectric material (GGX microfacet)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param world_out Out direction in world space
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo DielectricEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out, SampledWavelengths lambda) {
    MaterialEvalInfo m_info;
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum specular = GetFinalSpecular(info, lambda);
    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;
    float etai_over_etat = info.front_face ? (1.0f / GetInIOR(info)) : GetInIOR(info);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);
    vec3 l = normalize(world_out);
    
    // Compute half vector based on reflection or refraction
    vec3 h;
    bool is_reflect = dot(n, l) * dot(n, v) > 0.0f;
    if (is_reflect) {
        h = normalize(v + l);
    }
    else {
        h = -normalize(etai_over_etat * v + l);
        if (dot(n, h) < 0.0f) {
            h = -h;
        }
    }

    // PDF using the visible normal distribution
    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float pdf = Dv * abs(1.0f / (4.0f * dot(v, h)));

    float n_dot_v = abs(dot(n, v));
    float n_dot_l = abs(dot(n, l));

    float F = FresnelDielectric(v, h, etai_over_etat);
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    SampledSpectrum bsdf;
    if (is_reflect) {
        float dwh_dwi = abs(1.0f / (4.0f * dot(v, h)));
        m_info.pdf = F * Dv * dwh_dwi;

        bsdf = MulFloat(specular, F * D * G / (4.0f * n_dot_v * n_dot_l));
    }
    else {
        float h_dot_v = dot(h, v);
        float h_dot_l = dot(h, l);
        float sqrt_denom = etai_over_etat * h_dot_v + h_dot_l;
        float factor = abs(h_dot_l * h_dot_v / (n_dot_l * n_dot_v));

        float dwh_dwi = abs(h_dot_l) / (sqrt_denom * sqrt_denom);
        m_info.pdf = (1.0f - F) * Dv * dwh_dwi;

        // Mitsuba-style: solid angle compression factor for radiance transport
        // Only apply in Radiance mode (path tracing: light travels from eye to light)
        // In Importance mode (light tracing: light travels from light to eye), factor = 1
        float eta_factor = 1.0f;
        if (TransportMode_Radiance == info.transport_mode) {
            float inv_eta = 1.0f / etai_over_etat;
            eta_factor = inv_eta * inv_eta;
        }
        
        bsdf = MulFloat(specular, (1.0f - F) * D * G * factor * eta_factor / (sqrt_denom * sqrt_denom));
    }

    m_info.bsdf = bsdf;
    
    return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a dielectric material (GGX microfacet)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sobol_sampler Sobol sequence sampler (consumes 2 samples)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo DielectricSample(IntersectionInfo info, vec3 world_in, inout SobolSampler sobol_sampler, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    SampledSpectrum specular = GetFinalSpecular(info, lambda);
    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;
    float etai_over_etat = info.front_face ? (1.0f / GetInIOR(info)) : GetInIOR(info);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);

    // Sample visible normal distribution (returns local-space half vector)
    vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
    vec3 local_h = GGXSampleVisible(n, v, alpha_u, alpha_v, sample_xy);
    vec3 h = ToWorldFromUp(local_h, n);

    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float F = FresnelDielectric(v, h, etai_over_etat);
    float D = GGXD(h, n, alpha_u, alpha_v);

    SampledSpectrum bsdf;
    if (SobolSamplerGet1(sobol_sampler) < F) {
        // Reflection
        vec3 l = reflect(-v, h);

        float n_dot_v = dot(n, v);
        float n_dot_l = dot(n, l);

        if (n_dot_l <= 0.0f || n_dot_v <= 0.0f) {
            return m_info;
        }

        n_dot_v = abs(n_dot_v);
        n_dot_l = abs(n_dot_l);

        float dwh_dwi = abs(1.0f / (4.0f * dot(v, h)));
        m_info.pdf = F * Dv * dwh_dwi;

        float G = GGXG2(v, l, h, n, alpha_u, alpha_v);

        bsdf = MulFloat(specular, F * D * G / (4.0f * n_dot_v * n_dot_l));
        
        m_info.world_out = l;
    }
    else {
        // Refraction
        vec3 l = refract(-v, h, etai_over_etat);

        float n_dot_v = dot(n, v);
        float n_dot_l = dot(n, l);

        if (n_dot_l * n_dot_v >= 0.0f || length(l) < Epsilon) {
            return m_info;
        }

        n_dot_v = abs(n_dot_v);
        n_dot_l = abs(n_dot_l);

        float G = GGXG2(v, l, h, n, alpha_u, alpha_v);

        float h_dot_v = dot(h, v);
        float h_dot_l = dot(h, l);
        float sqrt_denom = etai_over_etat * h_dot_v + h_dot_l;
        float factor = abs(h_dot_l * h_dot_v / (n_dot_l * n_dot_v));

        float dwh_dwi = abs(h_dot_l) / (sqrt_denom * sqrt_denom);
        m_info.pdf = (1.0f - F) * Dv * dwh_dwi;

        // Mitsuba-style: solid angle compression factor for radiance transport
        // Only apply in Radiance mode (path tracing: light travels from eye to light)
        // In Importance mode (light tracing: light travels from light to eye), factor = 1
        float eta_factor = 1.0f;
        if (TransportMode_Radiance == info.transport_mode) {
            float inv_eta = 1.0f / etai_over_etat;
            eta_factor = inv_eta * inv_eta;
        }
        
        bsdf = MulFloat(specular, (1.0f - F) * D * G * factor * eta_factor / (sqrt_denom * sqrt_denom));
        
        m_info.world_out = l;
    }

    m_info.bsdf = bsdf;

    return m_info;
}

/**
 * @brief Evaluates the BSDF and PDF for a plastic material
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param world_out Out direction in world space
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo PlasticEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out, SampledWavelengths lambda) {
    MaterialEvalInfo m_info;
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    // Material properties
    SampledSpectrum kd = GetFinalDiffuse(info, lambda);    // Diffuse diffuse
    SampledSpectrum ks = GetFinalSpecular(info, lambda);   // Specular reflectance
    
    float d_sum = Sum(kd);
    float s_sum = Sum(ks);
    
    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;
    float eta = GetInIOR(info) / GetOutIOR(info);
    float F_avg = FresnelAverageDielectric(eta);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);
    vec3 l = normalize(world_out);
    vec3 h = normalize(v + l);

    float n_dot_v = dot(n, v);
    float n_dot_l = dot(n, l);
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f) {
        return m_info;
    }

    // Fresnel terms
    float Fo = FresnelDielectric(v, n, 1.0f / eta);
    float Fi = FresnelDielectric(l, n, 1.0f / eta);
    
    // Specular sampling weight (based on energy ratio)
    float specular_sampling_weight = s_sum / max(s_sum + d_sum, 1e-6f);
    float pdf_specular = Fi * specular_sampling_weight;
    float pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);
    pdf_specular = pdf_specular / max(pdf_specular + pdf_diffuse, 1e-6f);

    // GGX terms
    float F = FresnelDielectric(l, h, 1.0f / eta);
    float D = GGXD(h, n, alpha_u, alpha_v);
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);

    // Diffuse term (nonlinear energy-conserving)
    SampledSpectrum diffuse_brdf = Div(kd, Sub(SampledSpectrumNewFloat(1.0f), MulFloat(kd, F_avg)));
    diffuse_brdf = MulFloat(diffuse_brdf, (1.0f - Fi) * (1.0f - Fo) / PI);
    
    // Specular term
    SampledSpectrum specular_brdf = MulFloat(ks, F * D * G / (4.0f * n_dot_l * n_dot_v));

    m_info.bsdf = Add(diffuse_brdf, specular_brdf);
    
    // PDF: specular (VNDF) + diffuse (cosine)
    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float pdf_spec = Dv * abs(1.0f / (4.0f * dot(v, h)));
    float pdf_diff = CosineHemispherePDF(n_dot_l);
    m_info.pdf = pdf_specular * pdf_spec + (1.0f - pdf_specular) * pdf_diff;

    return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a plastic material
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sobol_sampler Sobol sequence sampler
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo PlasticSample(IntersectionInfo info, vec3 world_in, inout SobolSampler sobol_sampler, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    // Material properties
    SampledSpectrum kd = GetFinalDiffuse(info, lambda);
    SampledSpectrum ks = GetFinalSpecular(info, lambda);
    
    float d_sum = Sum(kd);
    float s_sum = Sum(ks);

    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;
    float eta = GetInIOR(info) / GetOutIOR(info);
    float F_avg = FresnelAverageDielectric(eta);

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);

    float n_dot_v = dot(n, v);
    if (n_dot_v <= 0.0f) {
        return m_info;
    }

    // Fresnel at view direction
    float Fo = FresnelDielectric(v, n, 1.0f / eta);
    float Fi = Fo;
    
    // Specular sampling weight
    float specular_sampling_weight = s_sum / max(s_sum + d_sum, 1e-6f);
    float pdf_specular = Fi * specular_sampling_weight;
    float pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);
    pdf_specular = pdf_specular / max(pdf_specular + pdf_diffuse, 1e-6f);

    vec3 l = vec3(0.0f);
    vec3 h = vec3(0.0f);
    float n_dot_l = 0.0f;

    // Sample specular or diffuse based on probability
    if (SobolSamplerGet1(sobol_sampler) < pdf_specular) {
        // Specular: VNDF sample half vector
        vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
        vec3 local_h = GGXSampleVisible(n, v, alpha_u, alpha_v, sample_xy);
        h = ToWorldFromUp(local_h, n);
        
        l = reflect(-v, h);
        
        n_dot_l = dot(n, l);
        if (n_dot_l <= 0.0f) {
            return m_info;
        }
    }
    else {
        // Diffuse: cosine hemisphere sample
        vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
        vec3 local_l = CosineHemisphereSample(sample_xy);
        l = ToWorldFromUp(local_l, n);
        
        h = normalize(v + l);
        Fi = FresnelDielectric(l, n, 1.0f / eta);
        
        n_dot_l = dot(n, l);
        if (n_dot_l <= 0.0f) {
            return m_info;
        }
    }

    // Evaluate BSDF at sampled direction
    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);
    float F = FresnelDielectric(l, h, 1.0f / eta);

    // Diffuse term
    SampledSpectrum diffuse_brdf = Div(kd, Sub(SampledSpectrumNewFloat(1.0f), MulFloat(kd, F_avg)));
    diffuse_brdf = MulFloat(diffuse_brdf, (1.0f - Fi) * (1.0f - Fo) / PI);
    
    // Specular term
    SampledSpectrum specular_brdf = MulFloat(ks, F * D * G / (4.0f * n_dot_l * n_dot_v));

    m_info.world_out = l;
    m_info.bsdf = Add(diffuse_brdf, specular_brdf);
    
    // PDF
    float pdf_spec = Dv * abs(1.0f / (4.0f * dot(v, h)));
    float pdf_diff = CosineHemispherePDF(n_dot_l);
    m_info.pdf = pdf_specular * pdf_spec + (1.0f - pdf_specular) * pdf_diff;

    return m_info;
}

/**
 * @brief Evaluates the BSDF and PDF for a metal workflow material (PBR metallic-roughness)
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param world_out Out direction in world space
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo MetalWorkflowEvaluate(IntersectionInfo info, vec3 world_in, vec3 world_out, SampledWavelengths lambda) {
    MaterialEvalInfo m_info;
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    // Material properties
    SampledSpectrum diffuse = GetFinalDiffuse(info, lambda);  // Base color / diffuse
    float metallic = GetFinalMetallic(info);
    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);
    vec3 l = normalize(world_out);
    vec3 h = normalize(v + l);

    float n_dot_v = dot(n, v);
    float n_dot_l = dot(n, l);
    if (n_dot_v <= 0.0f || n_dot_l <= 0.0f) {
        return m_info;
    }

    // Mix diffuse and specular based on metallic
    float dielectric_brdf = 1.0f - metallic;
    float diffuse_weight = dielectric_brdf;
    float specular_weight = metallic + dielectric_brdf;
    float denom = diffuse_weight + specular_weight;
    float p_diffuse = diffuse_weight / max(denom, 1e-6f);

    // GGX terms
    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    // Fresnel: mix between dielectric F0 (0.04) and diffuse based on metallic
    // F0 = mix(0.04, diffuse, metallic)
    SampledSpectrum F0 = MulFloat(diffuse, metallic);
    F0 = Add(MulFloat(SampledSpectrumNewFloat(0.04f), 1.0f - metallic), F0);
    
    // F = F0 + (1 - F0) * (1 - cosθ)^5
    float cos_theta = dot(v, h);
    float fresnel_factor = pow(1.0f - cos_theta, 5.0f);
    SampledSpectrum F = Add(F0, MulFloat(Sub(SampledSpectrumNewFloat(1.0f), F0), fresnel_factor));

    // Specular BRDF: D * F * G / (4 * NdotL * NdotV)
    SampledSpectrum specular_brdf = MulFloat(F, D * G / (4.0f * n_dot_l * n_dot_v));
    
    // Diffuse BRDF: diffuse / π
    SampledSpectrum diffuse_brdf = MulFloat(diffuse, 1.0f / PI);

    // Mix
    m_info.bsdf = Add(MulFloat(diffuse_brdf, p_diffuse), MulFloat(specular_brdf, 1.0f - p_diffuse));
    
    // PDF: specular (VNDF) + diffuse (cosine)
    float pdf_spec = Dv * abs(1.0f / (4.0f * dot(v, h)));
    float pdf_diff = CosineHemispherePDF(n_dot_l);
    m_info.pdf = (1.0f - p_diffuse) * pdf_spec + p_diffuse * pdf_diff;

    return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a metal workflow material
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sobol_sampler Sobol sequence sampler
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo MetalWorkflowSample(IntersectionInfo info, vec3 world_in, inout SobolSampler sobol_sampler, SampledWavelengths lambda) {
    MaterialSampleInfo m_info;
    m_info.world_out = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewFloat(0.0f);
    m_info.pdf = 0.0f;

    // Material properties
    SampledSpectrum diffuse = GetFinalDiffuse(info, lambda);
    float metallic = GetFinalMetallic(info);
    float roughness_u = GetFinalRoughnessU(info);
    float roughness_v = GetFinalRoughnessV(info);
    float alpha_u = roughness_u * roughness_u;
    float alpha_v = roughness_v * roughness_v;

    vec3 n = normalize(info.shading_normal);
    vec3 v = normalize(world_in);

    float n_dot_v = dot(n, v);
    if (n_dot_v <= 0.0f) {
        return m_info;
    }

    // Mix diffuse and specular based on metallic
    float dielectric_brdf = 1.0f - metallic;
    float diffuse_weight = dielectric_brdf;
    float specular_weight = metallic + dielectric_brdf;
    float denom = diffuse_weight + specular_weight;
    float p_diffuse = diffuse_weight / max(denom, 1e-6f);

    vec3 l = vec3(0.0f);
    vec3 h = vec3(0.0f);
    float n_dot_l = 0.0f;

    // Sample diffuse or specular based on probability
    if (SobolSamplerGet1(sobol_sampler) < p_diffuse) {
        // Diffuse: cosine hemisphere sample
        vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
        vec3 local_l = CosineHemisphereSample(sample_xy);
        l = ToWorldFromUp(local_l, n);
        
        h = normalize(v + l);
        
        n_dot_l = dot(n, l);
        if (n_dot_l <= 0.0f) {
            return m_info;
        }
    }
    else {
        // Specular: VNDF sample half vector
        vec2 sample_xy = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
        vec3 local_h = GGXSampleVisible(n, v, alpha_u, alpha_v, sample_xy);
        h = ToWorldFromUp(local_h, n);
        
        l = reflect(-v, h);
        
        n_dot_l = dot(n, l);
        if (n_dot_l <= 0.0f) {
            return m_info;
        }
    }

    // Evaluate BSDF at sampled direction
    float Dv = GGXDV(v, h, n, alpha_u, alpha_v);
    float G = GGXG2(v, l, h, n, alpha_u, alpha_v);
    float D = GGXD(h, n, alpha_u, alpha_v);

    // Fresnel: mix between dielectric F0 (0.04) and diffuse based on metallic
    SampledSpectrum F0 = MulFloat(diffuse, metallic);
    F0 = Add(MulFloat(SampledSpectrumNewFloat(0.04f), 1.0f - metallic), F0);
    
    float cos_theta = dot(v, h);
    float fresnel_factor = pow(1.0f - cos_theta, 5.0f);
    SampledSpectrum F = Add(F0, MulFloat(Sub(SampledSpectrumNewFloat(1.0f), F0), fresnel_factor));

    // Specular BRDF
    SampledSpectrum specular_brdf = MulFloat(F, D * G / (4.0f * n_dot_l * n_dot_v));
    
    // Diffuse BRDF
    SampledSpectrum diffuse_brdf = MulFloat(diffuse, 1.0f / PI);

    // Mix
    m_info.world_out = l;
    m_info.bsdf = Add(MulFloat(diffuse_brdf, p_diffuse), MulFloat(specular_brdf, 1.0f - p_diffuse));
    
    // PDF
    float pdf_spec = Dv * abs(1.0f / (4.0f * dot(v, h)));
    float pdf_diff = CosineHemispherePDF(n_dot_l);
    m_info.pdf = (1.0f - p_diffuse) * pdf_spec + p_diffuse * pdf_diff;

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
    else if (MaterialType_Dielectric == info.material.type) {
        return DielectricEvaluate(info, world_in, world_out, lambda);
    }
    else if (MaterialType_Plastic == info.material.type) {
        return PlasticEvaluate(info, world_in, world_out, lambda);
    }
    else if (MaterialType_MetalWorkflow == info.material.type) {
        return MetalWorkflowEvaluate(info, world_in, world_out, lambda);
    }
    
    // Unknown material type, return zero contribution
    return result;
}

/**
 * @brief Unified material sampling function that dispatches to the appropriate material model
 * @param info Intersection data containing material properties and surface normal
 * @param world_in In direction in world space
 * @param sobol_sampler Sobol sequence sampler (consumes 2 samples)
 * @param lambda Sampled wavelengths for spectral rendering
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo MaterialSample(IntersectionInfo info, vec3 world_in, inout SobolSampler sobol_sampler, SampledWavelengths lambda) {
    MaterialSampleInfo result;
    result.world_out = vec3(0.0f);
    result.bsdf = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Dispatch based on material type
    if (MaterialType_Diffuse == info.material.type) {
        return DiffuseSample(info, world_in, sobol_sampler, lambda);
    }
    else if (MaterialType_Conductor == info.material.type) {
        return ConductorSample(info, world_in, sobol_sampler, lambda);
    }
    else if (MaterialType_Dielectric == info.material.type) {
        return DielectricSample(info, world_in, sobol_sampler, lambda);
    }
    else if (MaterialType_Plastic == info.material.type) {
        return PlasticSample(info, world_in, sobol_sampler, lambda);
    }
    else if (MaterialType_MetalWorkflow == info.material.type) {
        return MetalWorkflowSample(info, world_in, sobol_sampler, lambda);
    }

    // Unknown material type, return zero contribution
    return result;
}

#endif // MATERIAL_GLSL