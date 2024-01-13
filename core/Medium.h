#pragma	once

#include "Utils.h"
#include "PhaseFunction.h"

enum MediumType {
	HomogeneousMedium
};

struct MediumParams {
	MediumType type;
	std::shared_ptr<PhaseFunction> phaseFunction;
	Spectrum sigma_s;
	Spectrum sigma_a;
	float scale;
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

	static std::shared_ptr<Medium> Create(const MediumParams& params);

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