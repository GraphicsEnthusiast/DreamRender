#include "Medium.h"

void Medium::EvaluateWavelength(const Spectrum& history, const Spectrum& albedo, std::vector<float>& pmf) {
	// Create empirical discrete distribution
	Spectrum history_albedo = history * albedo;
	std::vector<float> wave(Spectrum::nSamples);
	for (int i = 0; i < Spectrum::nSamples; i++) {
		wave[i] = history_albedo[i];
	}
	BinaryTable1D waveTable(wave);

	pmf.resize(Spectrum::nSamples);
	for (int i = 0; i < Spectrum::nSamples; i++) {
		pmf[i] = waveTable.GetPDF(i);
	}
}

int Medium::SampleWavelength(const Spectrum& history, const Spectrum& albedo, std::shared_ptr<Sampler> sampler, std::vector<float>& pmf) {
	// Create empirical discrete distribution
	Spectrum history_albedo = history * albedo;
	std::vector<float> wave(Spectrum::nSamples);
	for (int i = 0; i < Spectrum::nSamples; i++) {
		wave[i] = history_albedo[i];
	}
	BinaryTable1D waveTable(wave);

	pmf.resize(Spectrum::nSamples);
	for (int i = 0; i < Spectrum::nSamples; i++) {
		pmf[i] = waveTable.GetPDF(i);
	}

	// Sample index of wavelength from empirical discrete distribution
	int channel = waveTable.Sample(sampler->Get1());

	return channel;
}

Homogeneous::Homogeneous(std::shared_ptr<PhaseFunction> phase, const Spectrum& s, const Spectrum& a, float scale) :
	Medium(MediumType::HomogeneousMedium, phase), sigma_s(s * scale), sigma_t((a + s) * scale) {}

Spectrum Homogeneous::EvaluateDistance(const Spectrum& history, bool scattered, float distance, float& trans_pdf) {
	distance = std::min(MaxFloat, distance);
	scattered = false;
	trans_pdf = 0.0f;
	Spectrum transmittance(0.0f);

	std::vector<float> pmf_wavelength;
	EvaluateWavelength(history, (sigma_s / sigma_t), pmf_wavelength);

	if (!scattered) {
		transmittance = Exp(-sigma_t * distance);
		for (int i = 0; i < Spectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i];
		}

	}
	else {
		transmittance = Exp(-sigma_t * distance);
		for (int i = 0; i < Spectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i] * sigma_t[i];
		}

		scattered = true;
	}

	bool valid = false;
	for (int i = 0; i < Spectrum::nSamples; i++) {
		if (transmittance[i] > 0.0f) {
			valid = true;
		}
	}

	if (scattered) {
		transmittance *= sigma_s;
	}

	if (!valid) {
		transmittance = Spectrum(0.0f);
	}

	return transmittance;
}

Spectrum Homogeneous::SampleDistance(const Spectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) {
	distance = std::min(MaxFloat, distance);
	scattered = false;
	trans_pdf = 0.0f;
	Spectrum transmittance(0.0f);

	std::vector<float> pmf_wavelength;
	int channel = SampleWavelength(history, (sigma_s / sigma_t), sampler, pmf_wavelength);

	// Sample collision-free distance
	distance = -std::log(std::max(1.0f - sampler->Get1(), 0.0f)) / sigma_t[channel];

	// Hit volume boundary, no collision
	if (distance >= max_distance) {
		distance = max_distance;
		transmittance = Exp(-sigma_t * max_distance);
		for (int i = 0; i < Spectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i];
		}

		scattered = false;
	}
	else {
		transmittance = Exp(-sigma_t * distance);
		for (int i = 0; i < Spectrum::nSamples; i++) {
			trans_pdf += pmf_wavelength[i] * transmittance[i] * sigma_t[i];
		}

		scattered = true;
	}

	bool valid = false;
	for (int i = 0; i < Spectrum::nSamples; i++) {
		if (transmittance[i] > 0.0f) {
			valid = true;
		}
	}

	if (scattered) {
		transmittance *= sigma_s;
	}

	if (!valid) {
		transmittance = Spectrum(0.0f);
	}

	return transmittance;
}

Heterogeneous::Heterogeneous(std::shared_ptr<PhaseFunction> phase, std::shared_ptr<DensityGrid> grid,
	const Spectrum& absorption, const Spectrum& scattering, float densityMulti) : Medium(MediumType::HeterogeneousMedium, phase), densityGrid(grid),
	absorptionColor(absorption), scatteringColor(scattering), densityMultiplier(densityMulti) {
	// Compute wavelength independent majorant
	float max_density = GetMaxDensity();
	Spectrum m = absorptionColor * max_density + scatteringColor * max_density;
	majorant = std::max(m[0], std::max(m[1], m[2]));
	invMajorant = 1.0f / majorant;
}

Spectrum Heterogeneous::EvaluateDistance(const Spectrum& history, bool scattered, float distance, float& trans_pdf) {
	return Spectrum();
}

Spectrum Heterogeneous::SampleDistance(const Spectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) {
	return Spectrum();
}
