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

	virtual Spectrum EvaluateDistance(const Spectrum& history, bool scattered, float distance, float& trans_pdf) = 0;

	virtual Spectrum SampleDistance(const Spectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) = 0;

	static void EvaluateWavelength(const Spectrum& history, const Spectrum& albedo, std::vector<float>& pmf);

	static int SampleWavelength(const Spectrum& history, const Spectrum& albedo, std::shared_ptr<Sampler> sampler, std::vector<float>& pmf);

protected:
	MediumType m_type;
	std::shared_ptr<PhaseFunction> phaseFunction;
};

class Homogeneous : public Medium {
public:
	Homogeneous(std::shared_ptr<PhaseFunction> phase, const Spectrum& s, const Spectrum& a, float scale = 1.0f);

	virtual Spectrum EvaluateDistance(const Spectrum& history, bool scattered, float distance, float& trans_pdf) override;

	virtual Spectrum SampleDistance(const Spectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) override;

private:
	Spectrum sigma_s;
	Spectrum sigma_t;
};

class Heterogeneous : public Medium {
public:
	Heterogeneous(std::shared_ptr<PhaseFunction> phase, std::shared_ptr<DensityGrid> grid,
		const Spectrum& absorption,
		const Spectrum& scattering,
		float densityMulti = 1.0f);

	inline float GetDensity(const Point3f& p) const {
		return densityMultiplier * densityGrid->GetDensity(p);
	}

	inline float GetMaxDensity() const {
		return densityMultiplier * densityGrid->GetMaxDensity();
	}

	inline Spectrum GetSigma_a(float density) const {
		return absorptionColor * density;
	}

	inline Spectrum GetSigma_s(float density) const {
		return scatteringColor * density;
	}

	virtual Spectrum EvaluateDistance(const Spectrum& history, bool scattered, float distance, float& trans_pdf) override;

	virtual Spectrum SampleDistance(const Spectrum& history, float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<DensityGrid> densityGrid;
	Spectrum absorptionColor;
	Spectrum scatteringColor;
	float densityMultiplier;
	float majorant;
	float invMajorant;
};