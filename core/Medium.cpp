#include "Medium.h"

RGBSpectrum Isotropic::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	RGBSpectrum attenuation(INV_4PI);
    pdf = UniformPdfSphere();

	return attenuation;
}

RGBSpectrum Isotropic::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	RGBSpectrum attenuation(INV_4PI);
	L = UniformSampleSphere(sampler->Get2());
	pdf = UniformPdfSphere();

	return attenuation;
}

RGBSpectrum HenyeyGreenstein::Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	float cos_theta = glm::dot(L, V);
	RGBSpectrum attenuation(0.0f);
	pdf = 0.0f;
	for (int dim = 0; dim < 3; ++dim) {
		float temp = 1.0f + g[dim] * g[dim] + 2.0f * g[dim] * cos_theta;
		attenuation[dim] = INV_4PI * (1.0f - g[dim] * g[dim]) / (temp * std::sqrt(temp));
		pdf += attenuation[dim];
	}

	pdf *= (1.0f / 3.0f);

	if (pdf <= 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	return attenuation;
}

RGBSpectrum HenyeyGreenstein::Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	int channel = std::min(static_cast<int>(sampler->Get1() * 3), 2);
	float gc = g[channel];

	float cos_theta = 0.0f;
	if (std::abs(gc) < Epsilon) {
		cos_theta = 1.0f - 2.0f * sampler->Get1();
	}
	else {
		float sqr_term = (1.0f - gc * gc) / (1.0f - gc + 2.0f * gc * sampler->Get1());
		cos_theta = (1.0f + gc * gc - sqr_term * sqr_term) / (2.0f * gc);
	}

	pdf = 0.0f;
	RGBSpectrum attenuation = 0.0f;
	for (int dim = 0; dim < 3; ++dim) {
		float temp = 1.0f + g[dim] * g[dim] + 2.0f * g[dim] * cos_theta;
		attenuation[dim] = INV_PI * (1.0f - g[dim] * g[dim]) / (temp * std::sqrt(temp));
		pdf += attenuation[dim];
	}
	pdf *= (1.0f / 3.0f);

	if (pdf <= 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
	float phi = 2.0f * PI * sampler->Get1();

	Vector3f local_L = glm::normalize(Vector3f(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta));
	L = ToWorld(local_L, V);

	return attenuation;
}
