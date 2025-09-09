#ifndef _MATERIAL__GLSL__
#define _MATERIAL__GLSL__

#include "spectrum/spectrum.glsl"
#include "sample/sampling.glsl"

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
    vec3 world_l;
    SampledSpectrum bsdf;
    float pdf;
};

/**
 * @brief Evaluates the BSDF and PDF for a diffuse material(Oren-Nayar diffuse model calculation)
 * @param info Intersection data containing material properties and surface normal
 * @param world_v View direction in world space (pointing toward camera)
 * @param world_l Light direction in world space (pointing toward light source)
 * @return MaterialEvalInfo Structure containing BSDF and PDF values
 */
MaterialEvalInfo DiffuseEvaluate(IntersectionInfo info, vec3 world_v, vec3 world_l) {
    MaterialEvalInfo m_info;
    m_info.bsdf = SampledSpectrumNewZero();
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = info.material.diffuse;
	float roughness = info.material.roughness;

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
	SampledSpectrum brdf = MulFloat(diffuse, 1.0f / PI * (c1 + c2) * (1.0f + roughness * 0.5f)) ;
	float pdf = CosineHemispherePDF(n_dot_l);

    m_info.bsdf = brdf;
    m_info.pdf = pdf;

	return m_info;
}

/**
 * @brief Samples a direction and evaluates the BSDF for a diffuse material(Oren-Nayar diffuse model calculation)
 * @param info Intersection data containing material properties and surface normal
 * @param world_v View direction in world space (pointing toward camera)
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @return MaterialSampleInfo Structure containing sampled direction, BSDF, and PDF
 */
MaterialSampleInfo DiffuseSample(IntersectionInfo info, vec3 world_v, vec2 sample_xy) {
    MaterialSampleInfo m_info;
    m_info.world_l = vec3(0.0f);
    m_info.bsdf = SampledSpectrumNewZero();
    m_info.pdf = 0.0f;

    SampledSpectrum diffuse = info.material.diffuse;
    float roughness = info.material.roughness;

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
    m_info.bsdf = brdf;
    m_info.pdf = pdf;

    return m_info;
}

#endif // _MATERIAL__GLSL__