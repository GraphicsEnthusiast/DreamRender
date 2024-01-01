#include "Material.h"

RGBSpectrum Material::Emit(const Point2f& uv) {
	return RGBSpectrum(0.0f);
}

RGBSpectrum DiffuseLight::Emit(const Point2f& uv) {
	return emittedTexture->GetColor(uv);
}

RGBSpectrum DiffuseLight::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

RGBSpectrum DiffuseLight::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) {
	L = Vector3f(0.0f);
	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

RGBSpectrum Diffuse::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	RGBSpectrum albedo = albedoTexture->GetColor(info.uv);
	float roughness = roughnessTexture->GetColor(info.uv)[0];

	Vector3f N = info.Ns;
	Vector3f local_L = ToLocal(L, N);
	Vector3f H = glm::normalize(V + L);
	float NdotL = local_L.z;
	float NdotV = glm::dot(N, V);
	float VdotH = glm::dot(V, H);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		return RGBSpectrum(0.0f);
	}

	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float VdotL = 2.0f * VdotH * VdotH - 1.0f;
	float Cosri = VdotL - NdotV * NdotL;
	float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
	float C2 = 0.45f * s2 / (s2 + 0.09f) * Cosri * (Cosri >= 0.0f ? (std::max(NdotL, NdotV)) : 1.0f);

	RGBSpectrum brdf = albedo * INV_PI * (C1 + C2) * (1.0f + roughness * 0.5f);

	pdf = CosinePdfHemisphere(NdotL);

	return brdf;
}

RGBSpectrum Diffuse::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) {
	RGBSpectrum albedo = albedoTexture->GetColor(info.uv);
	float roughness = roughnessTexture->GetColor(info.uv)[0];

	Vector3f N = info.Ns;
	Vector3f local_L = CosineSampleHemisphere(sampler->Get2());
	L = ToWorld(local_L, N);
	Vector3f H = glm::normalize(V + L);
	float NdotL = local_L.z;
	float NdotV = glm::dot(N, V);
	float VdotH = glm::dot(V, H);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		return RGBSpectrum(0.0f);
	}

	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float VdotL = 2.0f * VdotH * VdotH - 1.0f;
	float Cosri = VdotL - NdotV * NdotL;
	float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
	float C2 = 0.45f * s2 / (s2 + 0.09f) * Cosri * (Cosri >= 0.0f ? (std::max(NdotL, NdotV)) : 1.0f);

	RGBSpectrum brdf = albedo * INV_PI * (C1 + C2) * (1.0f + roughness * 0.5f);

	pdf = CosinePdfHemisphere(NdotL);

	return brdf;
}
