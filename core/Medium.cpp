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

Homogeneous::Homogeneous(std::shared_ptr<PhaseFunction> phase, const RGBSpectrum& s, const RGBSpectrum& a, float sca) :
	Medium(MediumType::HomogeneousMedium, phase, sca), sigma_s(s * scale), sigma_t((a + s) * scale), medium_sampling_weight(0.0f) {
	RGBSpectrum albedo = sigma_s / sigma_t;
	for (int dim = 0; dim < 3; ++dim) {
		if (albedo[dim] > medium_sampling_weight && sigma_t[dim] != 0.0f) {
			medium_sampling_weight = albedo[dim];
		}
	}
	if (medium_sampling_weight > 0.0f) {
		medium_sampling_weight = std::max(medium_sampling_weight, 0.5f);
	}
}

RGBSpectrum Homogeneous::EvaluateDistance(bool scattered, float distance, float& trans_pdf) {
	distance = std::min(MaxFloat, distance);
	RGBSpectrum transmittance(0.0f);
	trans_pdf = 0.0f;
	bool valid = false;
	for (int dim = 0; dim < 3; ++dim) {
		transmittance[dim] = std::exp(-sigma_t[dim] * distance);
		if (transmittance[dim] > 0.0f) {
			valid = true;
		}
	}

	if (scattered) {
		for (int dim = 0; dim < 3; ++dim) {
			trans_pdf += sigma_t[dim] * transmittance[dim];
		}
		trans_pdf *= medium_sampling_weight * (1.0f / 3.0f);
		transmittance *= sigma_s;
	}
	else {
		for (int dim = 0; dim < 3; ++dim) {
			trans_pdf += transmittance[dim];
		}
		trans_pdf = medium_sampling_weight * (1.0f / 3.0f) * trans_pdf + (1.0f - medium_sampling_weight);
	}

	if (!valid) {
		transmittance = RGBSpectrum(0.0f);
	}

	return transmittance;
}

RGBSpectrum Homogeneous::SampleDistance(float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) {
	distance = std::min(MaxFloat, distance);
	scattered = false;
	float xi_1 = sampler->Get1();
	RGBSpectrum transmittance(0.0f);

	if (xi_1 < medium_sampling_weight) {
		xi_1 /= medium_sampling_weight;
		int channel = std::min(static_cast<int>(sampler->Get1() * 3), 2);
		distance = -std::log(1.0f - xi_1) / sigma_t[channel];
		if (distance < max_distance) {
			trans_pdf = 0.0f;
			for (int dim = 0; dim < 3; ++dim) {
				trans_pdf += sigma_t[dim] * exp(-sigma_t[dim] * distance);
			}
			trans_pdf *= medium_sampling_weight * (1.0f / 3.0f);
			scattered = true;
		}
	}

	if (!scattered) { 
		distance = max_distance;
		trans_pdf = 0.0f;
		for (int dim = 0; dim < 3; ++dim) {
			trans_pdf += exp(-sigma_t[dim] * distance);
		}
		trans_pdf = medium_sampling_weight * (1.0f / 3.0f) * trans_pdf + (1.0f - medium_sampling_weight);
	}

	bool valid = false;
	for (int dim = 0; dim < 3; ++dim) {
		(transmittance)[dim] = exp(-sigma_t[dim] * distance);
		if ((transmittance)[dim] > 0.0f) {
			valid = true;
		}
	}

	if (scattered) {
		transmittance *= sigma_s;
	}

	if (!valid) {
		transmittance = RGBSpectrum(0.0f);
	}

	return transmittance;
}
