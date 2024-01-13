#include "Material.h"

Vector3f NormalFromTangentToWorld(const Vector3f& n, Vector3f tangentNormal) {
	tangentNormal = glm::normalize(tangentNormal * 2.0f - 1.0f);

	// Orthonormal Basis
	Vector3f UpVector = std::abs(n.z) < 0.9f ? Vector3f(0.0f, 0.0f, 1.0f) : Vector3f(1.0f, 0.0f, 0.0f);
	Vector3f TangentX = glm::normalize(glm::cross(UpVector, n));
	Vector3f TangentY = glm::normalize(glm::cross(n, TangentX));

	return glm::normalize(TangentX * tangentNormal.x + TangentY * tangentNormal.y + n * tangentNormal.z);
}

Spectrum Material::Emit() {
	return Spectrum(0.0f);
}

Spectrum MediumBoundary::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	pdf = 0.0f;

	return Spectrum(0.0f);
}

Spectrum MediumBoundary::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	L = Vector3f(0.0f);
	pdf = 0.0f;

	return Spectrum(0.0f);
}

Spectrum DiffuseLight::Emit() {
	return radiance;
}

Spectrum DiffuseLight::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	pdf = 0.0f;

	return Spectrum(0.0f);
}

Spectrum DiffuseLight::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	L = Vector3f(0.0f);
	pdf = 0.0f;

	return Spectrum(0.0f);
}

Spectrum Diffuse::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float roughness = roughnessTexture->GetColor(info.uv)[0];

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f local_L = ToLocal(L, N);
	Vector3f H = glm::normalize(V + L);
	float NdotL = local_L.z;
	float NdotV = glm::dot(N, V);
	float VdotH = glm::dot(V, H);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float VdotL = 2.0f * VdotH * VdotH - 1.0f;
	float Cosri = VdotL - NdotV * NdotL;
	float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
	float C2 = 0.45f * s2 / (s2 + 0.09f) * Cosri * (Cosri >= 0.0f ? (std::max(NdotL, NdotV)) : 1.0f);

	Spectrum brdf = albedo * INV_PI * (C1 + C2) * (1.0f + roughness * 0.5f);

	pdf = CosinePdfHemisphere(NdotL);

	return brdf;
}

Spectrum Diffuse::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float roughness = roughnessTexture->GetColor(info.uv)[0];

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f local_L = CosineSampleHemisphere(sampler->Get2());
	L = ToWorld(local_L, N);
	Vector3f H = glm::normalize(V + L);
	float NdotL = local_L.z;
	float NdotV = glm::dot(N, V);
	float VdotH = glm::dot(V, H);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float VdotL = 2.0f * VdotH * VdotH - 1.0f;
	float Cosri = VdotL - NdotV * NdotL;
	float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
	float C2 = 0.45f * s2 / (s2 + 0.09f) * Cosri * (Cosri >= 0.0f ? (std::max(NdotL, NdotV)) : 1.0f);

	Spectrum brdf = albedo * INV_PI * (C1 + C2) * (1.0f + roughness * 0.5f);

	pdf = CosinePdfHemisphere(NdotL);
	
	return brdf;
}

Spectrum Conductor::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = glm::normalize(V + L);

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);

	if (NdotV <= 0.0f || NdotL <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	Spectrum F = Fresnel::FresnelConductor(V, H, eta, k);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);

	Spectrum brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);

	return brdf;
}

Spectrum Conductor::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sampler->Get2());
	H = ToWorld(H, N);
	L = glm::reflect(-V, H);

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);

	if (NdotV <= 0.0f || NdotL <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	Spectrum F = Fresnel::FresnelConductor(V, H, eta, k);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);

	Spectrum brdf = albedo * F * D * G / (4.0f * NdotV * NdotL);

	return brdf;
}

Spectrum Dielectric::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);
	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H;

	bool isReflect = glm::dot(N, L) * glm::dot(N, V) >= 0.0f;
	if (isReflect) {
		H = glm::normalize(V + L);
	}
	else {
		H = -glm::normalize(etai_over_etat * V + L);
		if (glm::dot(N, H) < 0.0f) {
			H = -H;
		}
	}

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);

	float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	Spectrum bsdf;
	if (isReflect) {
		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float dwh_dwi = std::abs(1.0f / (4.0f * glm::dot(V, H)));
		pdf = F * Dv * dwh_dwi;

		bsdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
	}
	else {
		if (NdotL * NdotV >= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float HdotV = glm::dot(H, V);
		float HdotL = glm::dot(H, L);
		float sqrtDenom = etai_over_etat * HdotV + HdotL;
		float factor = std::abs(HdotL * HdotV / (NdotL * NdotV));

		float dwh_dwi = std::abs(HdotL) / glm::pow2(sqrtDenom);
		pdf = (1.0f - F) * Dv * dwh_dwi;

		bsdf = albedo * (1.0f - F) * D * G * factor / glm::pow2(sqrtDenom);
		bsdf *= glm::pow2(1.0f / etai_over_etat);
	}

	return bsdf;
}

Spectrum Dielectric::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);
	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sampler->Get2());
	H = ToWorld(H, N);

	Spectrum bsdf;
	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	if (sampler->Get1() < F) {
		L = glm::reflect(-V, H);

		float NdotV = glm::dot(N, V);
		float NdotL = glm::dot(N, L);

		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float dwh_dwi = std::abs(1.0f / (4.0f * glm::dot(V, H)));
		pdf = F * Dv * dwh_dwi;

		float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);

		bsdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
	}
	else {
		L = glm::refract(-V, H, etai_over_etat);

		float NdotV = glm::dot(N, V);
		float NdotL = glm::dot(N, L);

		if (NdotL * NdotV >= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);

		float HdotV = glm::dot(H, V);
		float HdotL = glm::dot(H, L);
		float sqrtDenom = etai_over_etat * HdotV + HdotL;
		float factor = std::abs(HdotL * HdotV / (NdotL * NdotV));

		float dwh_dwi = std::abs(HdotL) / glm::pow2(sqrtDenom);
		pdf = (1.0f - F) * Dv * dwh_dwi;

		bsdf = albedo * (1.0f - F) * D * G * factor / glm::pow2(sqrtDenom);
		bsdf *= glm::pow2(1.0f / etai_over_etat);
	}

	return bsdf;
}

Spectrum Plastic::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum kd = albedoTexture->GetColor(info.uv);
	Spectrum ks = specularTexture->GetColor(info.uv);
	float d_sum = kd[0] + kd[1] + kd[2];
	float s_sum = ks[0] + ks[1] + ks[2];

	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);
	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);
	float F_avg = Fresnel::AverageFresnelDielectric(eta);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = glm::normalize(V + L);

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);
	if (NdotV <= 0.0f || NdotL <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	float Fo = Fresnel::FresnelDielectric(V, N, 1.0f / eta);
	float Fi = Fresnel::FresnelDielectric(L, N, 1.0f / eta);
	float specular_sampling_weight = s_sum / (s_sum + d_sum);
	float pdf_specular = Fi * specular_sampling_weight;
	float pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);
	pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

	Spectrum F = Fresnel::FresnelDielectric(L, H, 1.0f / eta);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);

	Spectrum brdf = 0.0f;
	Spectrum diffuse = kd, specular = ks;
	if (nonlinear) {
		brdf = diffuse / (Spectrum(1.0f) - diffuse * F_avg);
	}
	else {
		brdf = diffuse / (Spectrum(1.0f) - F_avg);
	}
	brdf *= (1.0f - Fi) * (1.0f - Fo) * INV_PI;
	brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

	pdf = pdf_specular * Dv * std::abs(1.0f / (4.0f * glm::dot(V, H))) + (1.0f - pdf_specular) * CosinePdfHemisphere(NdotL);

	return brdf;
}

Spectrum Plastic::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum kd = albedoTexture->GetColor(info.uv);
	Spectrum ks = specularTexture->GetColor(info.uv);
	float d_sum = kd[0] + kd[1] + kd[2];
	float s_sum = ks[0] + ks[1] + ks[2];

	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);
	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);
	float F_avg = Fresnel::AverageFresnelDielectric(eta);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	float Fo = Fresnel::FresnelDielectric(V, N, 1.0f / eta);
	float Fi = Fo;
	float specular_sampling_weight = s_sum / (s_sum + d_sum);
	float pdf_specular = Fi * specular_sampling_weight;
	float pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);
	pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

	Spectrum brdf(0.0f);
	Vector3f H;
	float NdotL = 0.0f;
	float NdotV = glm::dot(N, V);
	if (sampler->Get1() < pdf_specular) {
		H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sampler->Get2());
		H = ToWorld(H, N);
		L = glm::reflect(-V, H);

		NdotL = glm::dot(N, L);
		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}
	}
	else {
		Vector3f local_L = CosineSampleHemisphere(sampler->Get2());
		L = ToWorld(local_L, N);
		H = glm::normalize(V + L);
		Fi = Fresnel::FresnelDielectric(L, N, 1.0f / eta);

		NdotL = dot(N, L);
		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}
	}
	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	Spectrum F = Fresnel::FresnelDielectric(L, H, 1.0f / eta);

	Spectrum diffuse = kd, specular = ks;
	if (nonlinear) {
		brdf = diffuse / (Spectrum(1.0f) - diffuse * F_avg);
	}
	else {
		brdf = diffuse / (Spectrum(1.0f) - F_avg);
	}
	brdf *= (1.0f - Fi) * (1.0f - Fo) * INV_PI;
	brdf += specular * F * D * G / (4.0f * NdotL * NdotV);

	pdf = pdf_specular * Dv * std::abs(1.0f / (4.0f * glm::dot(V, H))) + (1.0f - pdf_specular) * CosinePdfHemisphere(NdotL);

	return brdf;
}

Spectrum ThinDielectric::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H;

	bool isReflect = glm::dot(N, L) * glm::dot(N, V) >= 0.0f;
	if (isReflect) {
		H = glm::normalize(V + L);
	}
	else {
		Vector3f Vr = ToWorld(ToLocal(V, -N), N);
		H = glm::normalize(Vr + L);
		if (glm::dot(N, H) < 0.0f) {
			H = -H;
		}
	}

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);

	float F = Fresnel::FresnelDielectric(V, H, 1.0f / eta);
	if (F < 1.0f) {
		F *= 2.0f / (1.0f + F);
	}
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	Spectrum bsdf(0.0f);
	float dwh_dwi = std::abs(1.0f / (4.0f * glm::dot(V, H)));
	if (isReflect) {
		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		pdf = F * Dv * dwh_dwi;

		bsdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
	}
	else {
		if (NdotL * NdotV >= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		pdf = (1.0f - F) * Dv * dwh_dwi;

		bsdf = albedo * (1.0f - F) * D * G / (4.0f * NdotV * NdotL);
	}

	return bsdf;
}

Spectrum ThinDielectric::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sampler->Get2());
	H = ToWorld(H, N);

	Spectrum bsdf;
	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	float F = Fresnel::FresnelDielectric(V, H, 1.0f / eta);
	if (F < 1.0f) {
		F *= 2.0f / (1.0f + F);
	}
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	if (sampler->Get1() < F) {
		L = glm::reflect(-V, H);

		float NdotV = glm::dot(N, V);
		float NdotL = glm::dot(N, L);

		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);

		float dwh_dwi = std::abs(1.0f / (4.0f * glm::dot(V, H)));
		pdf = F * Dv * dwh_dwi;

		float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);

		bsdf = albedo * F * D * G / (4.0f * NdotV * NdotL);
	}
	else {
		L = -V;

		float NdotV = glm::dot(N, V);
		float NdotL = glm::dot(N, L);

		if (NdotL * NdotV >= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		NdotV = std::abs(NdotV);
		NdotL = std::abs(NdotL);
		
		float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
		float dwh_dwi = std::abs(1.0f / (4.0f * glm::dot(V, H)));
		pdf = (1.0f - F) * Dv * dwh_dwi;

		bsdf = albedo * (1.0f - F) * D * G / (4.0f * NdotV * NdotL);
	}

	return bsdf;
}

Spectrum MetalWorkflow::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);
	float metallic = metallicTexture->GetColor(info.uv)[0];

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = glm::normalize(V + L);

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	float metallic_brdf = metallic;
	float dieletric_brdf = (1.0f - metallic);
	float diffuse = dieletric_brdf;
	float specular = metallic_brdf + dieletric_brdf;
	float deom = diffuse + specular;
	float p_diffuse = diffuse / deom;

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	Spectrum F0 = Lerp(metallic, Spectrum(0.04f), albedo);
	Spectrum F = Fresnel::FresnelSchlick(F0, glm::dot(V, H));

	Spectrum specular_brdf = D * F * G / (4.0f * NdotL * NdotV);
	Spectrum diffuse_brdf = albedo * INV_PI;
	Spectrum brdf = p_diffuse * diffuse_brdf + (1.0f - p_diffuse) * specular_brdf;

	pdf = (1.0f - p_diffuse) * Dv * std::abs(1.0f / (4.0f * glm::dot(V, H))) + p_diffuse * CosinePdfHemisphere(NdotL);

	return brdf;
}

Spectrum MetalWorkflow::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);
	float metallic = metallicTexture->GetColor(info.uv)[0];

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}

	float NdotV = glm::dot(N, V);

	float metallic_brdf = metallic;
	float dieletric_brdf = (1.0f - metallic);
	float diffuse = dieletric_brdf;
	float specular = metallic_brdf + dieletric_brdf;
	float deom = diffuse + specular;
	float p_diffuse = diffuse / deom;

	Vector3f H;
	float NdotL = 0.0f;
	if (sampler->Get1() < p_diffuse) {
		Vector3f local_L = CosineSampleHemisphere(sampler->Get2());
		L = ToWorld(local_L, N);
		H = glm::normalize(V + L);

		NdotL = glm::dot(N, L);

		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}
	}
	else {
		H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sampler->Get2());
		H = ToWorld(H, N);

		L = glm::reflect(-V, H);

		NdotL = glm::dot(N, L);

		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}
	}

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);
	Spectrum F0 = Lerp(metallic, Spectrum(0.04f), albedo);
	Spectrum F = Fresnel::FresnelSchlick(F0, glm::dot(V, H));

	Spectrum specular_brdf = D * F * G / (4.0f * NdotL * NdotV);
	Spectrum diffuse_brdf = albedo * INV_PI;
	Spectrum brdf = p_diffuse * diffuse_brdf + (1.0f - p_diffuse) * specular_brdf;

	pdf = (1.0f - p_diffuse) * Dv * std::abs(1.0f / (4.0f * glm::dot(V, H))) + p_diffuse * CosinePdfHemisphere(NdotL);

	return brdf;
}

Spectrum ClearCoatedConductor::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f H = glm::normalize(V + L);

	float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);

	float NdotV = glm::dot(N, V);
	float NdotL = glm::dot(N, L);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	float F = Fresnel::FresnelDielectric(V, H, 1.0f / 1.5f);
	float coat_weight = coatWeight * F;
	float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
	float D = GGX::Distribution(H, N, alpha_u, alpha_v);

	float cond_pdf = 0.0f;
	float coat_pdf = Dv * abs(1.0f / (4.0f * dot(V, H)));

	Spectrum cond_brdf = conductor->Evaluate(V, L, cond_pdf, info);
	Spectrum coat_brdf = D * G / (4.0f * NdotV * NdotL);
	Spectrum brdf = coat_weight * coat_brdf + (1.0f - coat_weight) * cond_brdf;

	pdf = coat_weight * coat_pdf + (1.0f - coat_weight) * cond_pdf;

	return brdf;
}

Spectrum ClearCoatedConductor::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	float alpha_u = glm::pow2(roughnessTexture_u->GetColor(info.uv)[0]);
	float alpha_v = glm::pow2(roughnessTexture_v->GetColor(info.uv)[0]);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}

	Vector3f H;
	float F = Fresnel::FresnelDielectric(V, N, 1.0f / 1.5f);
	float coat_weight = coatWeight * F;
	if (sampler->Get1() < coat_weight) {
		H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sampler->Get2());
		H = ToWorld(H, N);
		L = glm::reflect(-V, H);

		float NdotV = glm::dot(N, V);
		float NdotL = glm::dot(N, L);

		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
		F = Fresnel::FresnelDielectric(V, H, 1.0f / 1.5f);
		float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
		float D = GGX::Distribution(H, N, alpha_u, alpha_v);
		float coat_pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));
		float cond_pdf = 0.0f;

		Spectrum cond_brdf = conductor->Evaluate(V, L, cond_pdf, info);
		Spectrum coat_brdf = D * G / (4.0f * NdotV * NdotL);
		Spectrum brdf = coat_weight * coat_brdf + (1.0f - coat_weight) * cond_brdf;

		pdf = coat_weight * coat_pdf + (1.0f - coat_weight) * cond_pdf;

		return brdf;
	}
	else {
		float cond_pdf = 0.0f;
		Spectrum cond_brdf = conductor->Sample(V, L, cond_pdf, info, sampler);
		
		H = glm::normalize(V + L);

		float NdotV = glm::dot(N, V);
		float NdotL = glm::dot(N, L);

		if (NdotL <= 0.0f || NdotV <= 0.0f) {
			pdf = 0.0f;

			return Spectrum(0.0f);
		}

		float Dv = GGX::DistributionVisible(V, H, N, alpha_u, alpha_v);
		F = Fresnel::FresnelDielectric(V, H, 1.0f / 1.5f);
		float G = GGX::GeometrySmith1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith1(L, H, N, alpha_u, alpha_v);
		float D = GGX::Distribution(H, N, alpha_u, alpha_v);
		float coat_pdf = Dv * std::abs(1.0f / (4.0f * glm::dot(V, H)));

		Spectrum coat_brdf = D * G / (4.0f * NdotV * NdotL);
		Spectrum brdf = coat_weight * coat_brdf + (1.0f - coat_weight) * cond_brdf;

		pdf = coat_weight * coat_pdf + (1.0f - coat_weight) * cond_pdf;

		return brdf;
	}
}

Spectrum DiffuseTransmitter::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}

	float NdotL = glm::dot(N, L);
	float NdotV = glm::dot(N, V);

	if (NdotL * NdotV >= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	pdf = CosinePdfHemisphere(std::abs(NdotL));

	return albedo * INV_PI;
}

Spectrum DiffuseTransmitter::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Spectrum albedo = albedoTexture->GetColor(info.uv);

	Vector3f N = info.Ns;
	if (normalTexture != NULL) {
		Spectrum tangentNormal = normalTexture->GetColor(info.uv);
		N = NormalFromTangentToWorld(N, Vector3f(tangentNormal[0], tangentNormal[1], tangentNormal[2]));
	}
	Vector3f local_L = CosineSampleHemisphere(sampler->Get2());
	N = -N;
	L = ToWorld(local_L, N);
	float NdotL = local_L.z;
	float NdotV = glm::dot(N, V);

	if (NdotL * NdotV >= 0.0f) {
		pdf = 0.0f;

		return Spectrum(0.0f);
	}

	Spectrum btdf = albedo * INV_PI;

	pdf = CosinePdfHemisphere(std::abs(NdotL));

	return btdf;
}

Spectrum Mixture::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	float pdf1 = 0.0f;
	Spectrum bsdf1 = material1->Evaluate(V, L, pdf1, info);
	float pdf2 = 0.0f;
	Spectrum bsdf2 = material1->Evaluate(V, L, pdf2, info);

	pdf = weight * pdf1 + (1.0f - weight) * pdf2;
	Spectrum bsdf = weight * bsdf1 + (1.0f - weight) * bsdf2;

	return bsdf;
}

Spectrum Mixture::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	float pdf1 = 0.0f;
	Vector3f L1;
	Spectrum bsdf1 = material1->Sample(V, L1, pdf1, info, sampler);
	float pdf2 = 0.0f;
	Vector3f L2;
	Spectrum bsdf2 = material1->Sample(V, L2, pdf2, info, sampler);

	if (sampler->Get1() < weight) {
		L = L1;
	}
	else {
		L = L2;
	}

	pdf = weight * pdf1 + (1.0f - weight) * pdf2;
	Spectrum bsdf = weight * bsdf1 + (1.0f - weight) * bsdf2;

	return bsdf;
}

std::shared_ptr<Material> Material::Create(const MaterialParams& params) {
	if (params.type == MaterialType::MediumBoundaryMaterial) {
		return std::make_shared<MediumBoundary>();
	}
	else if (params.type == MaterialType::DiffuseLightMaterial) {
		return std::make_shared<DiffuseLight>(params.radiance);
	}
	else if (params.type == MaterialType::DiffuseMaterial) {
		return std::make_shared<Diffuse>(params.albedoTexture, params.roughnessTexture, params.normalTexture);
	}
	else if (params.type == MaterialType::ConductorMaterial) {
		return std::make_shared<Conductor>(params.albedoTexture, params.roughnessTexture_u, params.roughnessTexture_v, 
			params.eta, params.k, params.normalTexture);
	}
	else if (params.type == MaterialType::DielectricMaterial) {
		return std::make_shared<Dielectric>(params.albedoTexture, params.roughnessTexture_u, params.roughnessTexture_v, 
			params.int_ior, params.ext_ior, params.normalTexture);
	}
	else if (params.type == MaterialType::PlasticMaterial) {
		return std::make_shared<Plastic>(params.albedoTexture, params.specularTexture, params.roughnessTexture_u, params.roughnessTexture_v,
			params.int_ior, params.ext_ior, params.nonlinear, params.normalTexture);
	}
	else if (params.type == MaterialType::ThinDielectricMaterial) {
		return std::make_shared<ThinDielectric>(params.albedoTexture, params.roughnessTexture_u, params.roughnessTexture_v,
			params.int_ior, params.ext_ior, params.normalTexture);
	}
	else if (params.type == MaterialType::MetalWorkflowMaterial) {
		return std::make_shared<MetalWorkflow>(params.albedoTexture, params.roughnessTexture_u, params.roughnessTexture_v, 
			params.metallicTexture, params.normalTexture);
	}
	else if (params.type == MaterialType::ClearCoatedConductorMaterial) {
		return std::make_shared<ClearCoatedConductor>(params.conductor, params.roughnessTexture_u, params.roughnessTexture_v, 
			params.coatWeight, params.normalTexture);
	}
	else if(params.type == MaterialType::DiffuseTransmitterMaterial) {
		std::make_shared<DiffuseTransmitter>(params.albedoTexture, params.normalTexture);
	}
	else if (params.type == MaterialType::MixtureMaterial) {
		return std::make_shared<Mixture>(params.material1, params.material2, params.weight);
	}

	return NULL;
}