#include "Medium.h"

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
