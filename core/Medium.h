#pragma	once

#include "Utils.h"
#include "PhaseFunction.h"
#include "DensityGrid.h"

enum MediumType {
	HomogeneousMedium,
	HeterogeneousMedium
};

class Medium {
public:
	Medium(MediumType type, std::shared_ptr<PhaseFunction> phase) : m_type(type), phaseFunction(phase) {}

	inline MediumType GetType() const {
		return m_type;
	}

	inline std::shared_ptr<PhaseFunction> GetPhaseFunction() const {
		return phaseFunction;
	}

	virtual RGBSpectrum EvaluateDistance(const RGBSpectrum& history, bool scattered, float distance, float& trans_pdf) = 0;

	virtual RGBSpectrum SampleDistance(const RGBSpectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) = 0;

	static void EvaluateWavelength(const RGBSpectrum& history, const RGBSpectrum& albedo, std::vector<float>& pmf);

	static int SampleWavelength(const RGBSpectrum& history, const RGBSpectrum& albedo, std::shared_ptr<Sampler> sampler, std::vector<float>& pmf);

protected:
	MediumType m_type;
	std::shared_ptr<PhaseFunction> phaseFunction;
};

class Homogeneous : public Medium {
public:
	Homogeneous(std::shared_ptr<PhaseFunction> phase, const RGBSpectrum& s, const RGBSpectrum& a, float scale = 1.0f);

	virtual RGBSpectrum EvaluateDistance(const RGBSpectrum& history, bool scattered, float distance, float& trans_pdf) override;

	virtual RGBSpectrum SampleDistance(const RGBSpectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) override;

private:
	RGBSpectrum sigma_s;
	RGBSpectrum sigma_t;
};

class Heterogeneous : public Medium {
public:
	Heterogeneous(std::shared_ptr<PhaseFunction> phase, std::shared_ptr<DensityGrid> grid,
		const RGBSpectrum& absorption,
		const RGBSpectrum& scattering,
		float densityMulti = 1.0f);

	inline float GetDensity(const Point3f& p) const {
		return densityMultiplier * densityGrid->GetDensity(p);
	}

	inline float GetMaxDensity() const {
		return densityMultiplier * densityGrid->GetMaxDensity();
	}

	inline RGBSpectrum GetSigma_a(float density) const {
		return absorptionColor * density;
	}

	inline RGBSpectrum GetSigma_s(float density) const {
		return scatteringColor * density;
	}

	virtual RGBSpectrum EvaluateDistance(const RGBSpectrum& history, bool scattered, float distance, float& trans_pdf) override;

	virtual RGBSpectrum SampleDistance(const RGBSpectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<DensityGrid> densityGrid;
	RGBSpectrum absorptionColor;
	RGBSpectrum scatteringColor;
	float densityMultiplier;
	float majorant;
	float invMajorant;
};