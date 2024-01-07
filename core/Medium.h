#pragma	once

#include "Utils.h"
#include "PhaseFunction.h"

enum MediumType {
	HomogeneousMedium
};

class Medium {
public:
	Medium(MediumType type, std::shared_ptr<PhaseFunction> phase, float sca = 1.0f) : m_type(type), scale(sca), phaseFunction(phase) {}

	inline MediumType GetType() const {
		return m_type;
	}

	inline std::shared_ptr<PhaseFunction> GetPhaseFunction() const {
		return phaseFunction;
	}

	virtual RGBSpectrum EvaluateDistance(bool scattered, float distance, float& trans_pdf) = 0;

	virtual RGBSpectrum SampleDistance(float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) = 0;

protected:
	MediumType m_type;
	float scale;
	std::shared_ptr<PhaseFunction> phaseFunction;
};

class Homogeneous : public Medium {
public:
	Homogeneous(std::shared_ptr<PhaseFunction> phase, const RGBSpectrum& s, const RGBSpectrum& a, float sca = 1.0f);

	virtual RGBSpectrum EvaluateDistance(bool scattered, float distance, float& trans_pdf) override;

	virtual RGBSpectrum SampleDistance(float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) override;

private:
	float medium_sampling_weight;
	RGBSpectrum sigma_s;
	RGBSpectrum sigma_t;
};