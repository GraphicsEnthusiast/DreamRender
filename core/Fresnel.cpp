#include "Fresnel.h"

float Fresnel::FresnelSchlick(float f0, float VdotH) {
	float tmp = 1.0f - glm::clamp(VdotH, 0.0f, 1.0f);
	float tmp2 = tmp * tmp;
	float Fc = tmp2 * tmp2 * tmp;

	return f0 + (1.0f - f0) * Fc;
}

RGBSpectrum Fresnel::FresnelSchlick(const RGBSpectrum& f0, float VdotH) {
	float tmp = 1.0f - glm::clamp(VdotH, 0.0f, 1.0f);
	float tmp2 = tmp * tmp;
	float Fc = tmp2 * tmp2 * tmp;

	return f0 + (RGBSpectrum(1.0f) - f0) * Fc;
}

RGBSpectrum Fresnel::FresnelConductor(const Vector3f& V, const Vector3f& H, const RGBSpectrum& eta_r, const RGBSpectrum& eta_i) {
	Vector3f N = H;
	float cos_v_n = glm::dot(V, N),
		cos_v_n_2 = cos_v_n * cos_v_n,
		sin_v_n_2 = 1.0f - cos_v_n_2,
		sin_v_n_4 = sin_v_n_2 * sin_v_n_2;

	RGBSpectrum temp_1 = eta_r * eta_r - eta_i * eta_i - sin_v_n_2,
		a_2_pb_2 = temp_1 * temp_1 + 4.0f * eta_i * eta_i * eta_r * eta_r;
	for (int i = 0; i < 3; i++) {
		a_2_pb_2[i] = std::sqrt(std::max(0.0f, a_2_pb_2[i]));
	}
	RGBSpectrum a = 0.5f * (a_2_pb_2 + temp_1);
	for (int i = 0; i < 3; i++) {
		a[i] = std::sqrt(std::max(0.0f, a[i]));
	}
	RGBSpectrum term_1 = a_2_pb_2 + sin_v_n_2,
		term_2 = 2.0f * cos_v_n * a,
		term_3 = a_2_pb_2 * cos_v_n_2 + sin_v_n_4,
		term_4 = term_2 * sin_v_n_2,
		r_s = (term_1 - term_2) / (term_1 + term_2),
		r_p = r_s * (term_3 - term_4) / (term_3 + term_4);

	return 0.5f * (r_s + r_p);
}

RGBSpectrum Fresnel::AverageFresnelConductor(const RGBSpectrum& eta, const RGBSpectrum& k) {
	auto reflectivity = RGBSpectrum(0.0f),
		edgetint = RGBSpectrum(0.0f);
	float temp1 = 0.0f, temp2 = 0.0f, temp3 = 0.0f;
	for (int i = 0; i < 3; i++) {
		reflectivity[i] = (glm::pow2(eta[i] - 1.0f) + glm::pow2(k[i])) / (glm::pow2(eta[i] + 1.0f) + glm::pow2(k[i]));
		temp1 = 1.0f + std::sqrt(reflectivity[i]);
		temp2 = 1.0f - std::sqrt(reflectivity[i]);
		temp3 = (1.0f - reflectivity[i]) / (1.0f + reflectivity[i]);
		edgetint[i] = (temp1 - eta[i] * temp2) / (temp1 - temp3 * temp2);
	}

	return RGBSpectrum(0.087237f) +
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