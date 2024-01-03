#include "Material.h"

float Fresnel::FresnelSchlick(float f0, float VdotH) {
	float tmp = 1.0f - glm::clamp(VdotH, 0.0f, 1.0f);
	float tmp2 = tmp * tmp;
	float Fc = tmp2 * tmp2 * tmp;

	return f0 + (1.0f - f0) * Fc;
}

Vector3f Fresnel::FresnelSchlick(Vector3f f0, float VdotH) {
	float tmp = 1.0f - glm::clamp(VdotH, 0.0f, 1.0f);
	float tmp2 = tmp * tmp;
	float Fc = tmp2 * tmp2 * tmp;

	return f0 + (1.0f - f0) * Fc;
}

Vector3f Fresnel::FresnelConductor(const Vector3f& V, const Vector3f& H, const Vector3f& eta_r, const Vector3f& eta_i) {
	Vector3f N = H;
	float cos_v_n = glm::dot(V, N),
		cos_v_n_2 = cos_v_n * cos_v_n,
		sin_v_n_2 = 1.0f - cos_v_n_2,
		sin_v_n_4 = sin_v_n_2 * sin_v_n_2;

	Vector3f temp_1 = eta_r * eta_r - eta_i * eta_i - sin_v_n_2,
		a_2_pb_2 = temp_1 * temp_1 + 4.0f * eta_i * eta_i * eta_r * eta_r;
	for (int i = 0; i < 3; i++) {
		a_2_pb_2[i] = std::sqrt(std::max(0.0f, a_2_pb_2[i]));
	}
	Vector3f a = 0.5f * (a_2_pb_2 + temp_1);
	for (int i = 0; i < 3; i++) {
		a[i] = std::sqrt(std::max(0.0f, a[i]));
	}
	Vector3f term_1 = a_2_pb_2 + sin_v_n_2,
		term_2 = 2.0f * cos_v_n * a,
		term_3 = a_2_pb_2 * cos_v_n_2 + sin_v_n_4,
		term_4 = term_2 * sin_v_n_2,
		r_s = (term_1 - term_2) / (term_1 + term_2),
		r_p = r_s * (term_3 - term_4) / (term_3 + term_4);

	return 0.5f * (r_s + r_p);
}

Vector3f Fresnel::AverageFresnelConductor(Vector3f eta, Vector3f k) {
	auto reflectivity = Vector3f(0.0f),
		edgetint = Vector3f(0.0f);
	float temp1 = 0.0f, temp2 = 0.0f, temp3 = 0.0f;
	for (int i = 0; i < 3; i++) {
		reflectivity[i] = (glm::pow2(eta[i] - 1.0f) + glm::pow2(k[i])) / (glm::pow2(eta[i] + 1.0f) + glm::pow2(k[i]));
		temp1 = 1.0f + std::sqrt(reflectivity[i]);
		temp2 = 1.0f - std::sqrt(reflectivity[i]);
		temp3 = (1.0f - reflectivity[i]) / (1.0f + reflectivity[i]);
		edgetint[i] = (temp1 - eta[i] * temp2) / (temp1 - temp3 * temp2);
	}

	return Vector3f(0.087237f) +
		0.0230685f * edgetint -
		0.0864902f * edgetint * edgetint +
		0.0774594f * edgetint * edgetint * edgetint +
		0.782654f * reflectivity -
		0.136432f * reflectivity * reflectivity +
		0.278708f * reflectivity * reflectivity * reflectivity +
		0.19744f * edgetint * reflectivity +
		0.0360605f * edgetint * edgetint * reflectivity -
		0.2586f * edgetint * reflectivity * reflectivity;
}

float Fresnel::FresnelDielectric(const Vector3f& V, const Vector3f& H, float eta_inv) {
	float cos_theta_i = std::abs(glm::dot(V, H));
	float cos_theta_t_2 = 1.0f - glm::pow2(eta_inv) * (1.0f - glm::pow2(cos_theta_i));
	if (cos_theta_t_2 <= 0.0f) {
		return 1.0f;
	}
	else {
		float cos_theta_t = std::sqrt(cos_theta_t_2),
			Rs_sqrt = (eta_inv * cos_theta_i - cos_theta_t) / (eta_inv * cos_theta_i + cos_theta_t),
			Rp_sqrt = (cos_theta_i - eta_inv * cos_theta_t) / (cos_theta_i + eta_inv * cos_theta_t);

		return (Rs_sqrt * Rs_sqrt + Rp_sqrt * Rp_sqrt) / 2.0f;
	}
}

float Fresnel::AverageFresnelDielectric(float eta) {
	if (eta < 1.0f) {
		/* Fit by Egan and Hilgeman (1973). Vrks reasonably well for
			"normal" IOR values (<2).
			Max rel. error in 1.0 - 1.5 : 0.1%
			Max rel. error in 1.5 - 2   : 0.6%
			Max rel. error in 2.0 - 5   : 9.5%
		*/
		return -1.4399f * (eta * eta) + 0.7099f * eta + 0.6681f + 0.0636f / eta;
	}
	else {
		/* Fit by d'Eon and Irving (2011)

			Maintains a good accuracy even for unistic IOR values.

			Max rel. error in 1.0 - 2.0   : 0.1%
			Max rel. error in 2.0 - 10.0  : 0.2%
		*/
		float inv_eta = 1.0f / eta,
			inv_eta_2 = inv_eta * inv_eta,
			inv_eta_3 = inv_eta_2 * inv_eta,
			inv_eta_4 = inv_eta_3 * inv_eta,
			inv_eta_5 = inv_eta_4 * inv_eta;

		return 0.919317f - 3.4793f * inv_eta + 6.75335f * inv_eta_2 - 7.80989f * inv_eta_3 + 4.98554f * inv_eta_4 - 1.36881f * inv_eta_5;
	}
}

float GGX::GeometrySmith1(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v) {
	float cos_v_n = glm::dot(V, N);

	if (cos_v_n * glm::dot(V, H) <= 0.0f) {
		return 0.0f;
	}

	if (std::abs(cos_v_n - 1.0f) < 1e-4f) {
		return 1.0f;
	}

	if (alpha_u == alpha_v) {
		float cos_v_n_2 = glm::pow2(cos_v_n),
			tan_v_n_2 = (1.0f - cos_v_n_2) / cos_v_n_2,
			alpha_2 = alpha_u * alpha_u;

		return 2.0f / (1.0f + std::sqrt(1.0f + alpha_2 * tan_v_n_2));
	}
	else {
		Vector3f dir = ToLocal(V, N);
		float xy_alpha_2 = glm::pow2(alpha_u * dir.x) + glm::pow2(alpha_v * dir.y),
			tan_v_n_alpha_2 = xy_alpha_2 / glm::pow2(dir.z);

		return 2.0f / (1.0f + std::sqrt(1.0f + tan_v_n_alpha_2));
	}
}

float GGX::DistributionGGX(const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v) {
	float cos_theta = dot(H, N);
	if (cos_theta <= 0.0f) {
		return 0.0f;
	}
	float cos_theta_2 = glm::pow2(cos_theta),
		tan_theta_2 = (1.0f - cos_theta_2) / cos_theta_2,
		alpha_2 = alpha_u * alpha_v;
	if (alpha_u == alpha_v) {
		return alpha_2 / (PI * glm::pow2(cos_theta) * glm::pow2(alpha_2 + tan_theta_2));
	}
	else {
		Vector3f dir = ToLocal(H, N);

		return 1.0f / (PI * alpha_2 * glm::pow2(glm::pow2(dir.x / alpha_u) + glm::pow2(dir.y / alpha_v) + glm::pow2(dir.z)));
	}
}

float GGX::DistributionVisibleGGX(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v) {
	return GeometrySmith1(V, H, N, alpha_u, alpha_v) * glm::dot(V, H) * DistributionGGX(H, N, alpha_u, alpha_v) / glm::dot(N, V);
}

Vector3f GGX::SampleVisibleGGX(const Vector3f& N, const Vector3f& Ve, float alpha_u, float alpha_v, const Point2f& sample) {
	Vector3f V = ToLocal(Ve, N);
	Vector3f Vh = glm::normalize(Vector3f(alpha_u * V.x, alpha_v * V.y, V.z));

	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	float len2 = glm::pow2(Vh.x) + glm::pow2(Vh.y);
	Vector3f T1 = len2 > 0.0f ? Vector3f(-Vh.y, Vh.x, 0.0f) * glm::inversesqrt(len2) : Vector3f(1.0f, 0.0f, 0.0f);
	Vector3f T2 = glm::cross(Vh, T1);

	// Section 4.2: parameterization of the projected area
	float u = sample.x;
	float v = sample.y;
	float r = std::sqrt(u);
	float phi = v * 2.0f * PI;
	float t1 = r * std::cos(phi);
	float t2 = r * std::sin(phi);
	float s = 0.5f * (1.0f + Vh.z);
	t2 = (1.0f - s) * std::sqrt(1.0f - glm::pow2(t1)) + s * t2;

	// Section 4.3: reprojection onto hemisphere
	Vector3f Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0f, 1.0f - glm::pow2(t1) - glm::pow2(t2))) * Vh;

	// Section 3.4: transforming the normal back to the ellipsoid configuration
	Vector3f H = glm::normalize(Vector3f(alpha_u * Nh.x, alpha_v * Nh.y, std::max(0.0f, Nh.z)));

	return H;
}

RGBSpectrum Material::Emit() {
	return RGBSpectrum(0.0f);
}

RGBSpectrum DiffuseLight::Emit() {
	return radiance;
}

RGBSpectrum DiffuseLight::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

RGBSpectrum DiffuseLight::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
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
		pdf = 0.0f;

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

RGBSpectrum Diffuse::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
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
		pdf = 0.0f;

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
