#include "Medium.h"

void Medium::EvaluateWavelength(const RGBSpectrum& history, const RGBSpectrum& albedo, std::vector<float>& pmf) {
	// Create empirical discrete distribution
	const RGBSpectrum history_albedo = history * albedo;
	std::vector<float> wave(RGBSpectrum::nSamples);
	for (int i = 0; i < RGBSpectrum::nSamples; i++) {
		wave[i] = history_albedo[i];
	}
	BinaryTable1D waveTable(wave);

	pmf.resize(RGBSpectrum::nSamples);
	for (int i = 0; i < RGBSpectrum::nSamples; i++) {
		pmf[i] = waveTable.GetPDF(i);
	}
}

int Medium::SampleWavelength(const RGBSpectrum& history, const RGBSpectrum& albedo, std::shared_ptr<Sampler> sampler, std::vector<float>& pmf) {
	// Create empirical discrete distribution
	const RGBSpectrum history_albedo = history * albedo;
	std::vector<float> wave(RGBSpectrum::nSamples);
	for (int i = 0; i < RGBSpectrum::nSamples; i++) {
		wave[i] = history_albedo[i];
	}
	BinaryTable1D waveTable(wave);

	pmf.resize(RGBSpectrum::nSamples);
	for (int i = 0; i < RGBSpectrum::nSamples; i++) {
		pmf[i] = waveTable.GetPDF(i);
	}

	// Sample index of wavelength from empirical discrete distribution
	int channel = waveTable.Sample(sampler->Get1());

	return channel;
}

Homogeneous::Homogeneous(std::shared_ptr<PhaseFunction> phase, const RGBSpectrum& s, const RGBSpectrum& a, float scale) :
	Medium(MediumType::HomogeneousMedium, phase), sigma_s(s* scale), sigma_t((a + s)* scale) {}

RGBSpectrum Homogeneous::EvaluateDistance(const RGBSpectrum& history, bool scattered, float distance, float& trans_pdf) {
	distance = std::min(MaxFloat, distance);
	scattered = false;
	trans_pdf = 0.0f;
	RGBSpectrum transmittance(0.0f);

	std::vector<float> pmf_wavelength;
	EvaluateWavelength(history, (sigma_s / sigma_t), pmf_wavelength);

	if (!scattered) {
		transmittance = Exp(-sigma_t * distance);
		for (int i = 0; i < RGBSpectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i];
		}

	}
	else {
		transmittance = Exp(-sigma_t * distance);
		for (int i = 0; i < RGBSpectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i] * sigma_t[i];
		}

		scattered = true;
	}

	bool valid = false;
	for (int dim = 0; dim < RGBSpectrum::nSamples; ++dim) {
		if (transmittance[dim] > 0.0f) {
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

RGBSpectrum Homogeneous::SampleDistance(const RGBSpectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) {
	distance = std::min(MaxFloat, distance);
	scattered = false;
	trans_pdf = 0.0f;
	RGBSpectrum transmittance(0.0f);

	std::vector<float> pmf_wavelength;
	int channel = SampleWavelength(history, (sigma_s / sigma_t), sampler, pmf_wavelength);

	// Sample collision-free distance
	distance = -std::log(std::max(1.0f - sampler->Get1(), 0.0f)) / sigma_t[channel];

	// Hit volume boundary, no collision
	if (distance >= max_distance) {
		distance = max_distance;
		transmittance = Exp(-sigma_t * max_distance);
		for (int i = 0; i < RGBSpectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i];
		}

		scattered = false;
	}
	else {
		transmittance = Exp(-sigma_t * distance);
		for (int i = 0; i < RGBSpectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i] * sigma_t[i];
		}

		scattered = true;
	}

	bool valid = false;
	for (int dim = 0; dim < RGBSpectrum::nSamples; ++dim) {
		if (transmittance[dim] > 0.0f) {
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

Heterogeneous::Heterogeneous(std::shared_ptr<PhaseFunction> phase, std::shared_ptr<DensityGrid> grid,
	const RGBSpectrum& absorption, const RGBSpectrum& scattering,
	float densityMulti) : Medium(MediumType::HeterogeneousMedium, phase), densityGrid(grid),
	absorptionColor(absorption), scatteringColor(scattering), densityMultiplier(densityMulti) {
	// Compute wavelength independent majorant
	float max_density = GetMaxDensity();
	RGBSpectrum m = absorptionColor * max_density + scatteringColor * max_density;
	majorant = std::max(m[0], std::max(m[1], m[2]));
	invMajorant = 1.0f / majorant;
}

RGBSpectrum Heterogeneous::EvaluateDistance(const RGBSpectrum& history, bool scattered, float distance, float& trans_pdf) {
	return RGBSpectrum();
}

RGBSpectrum Heterogeneous::SampleDistance(const RGBSpectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) {
	return RGBSpectrum();
}
