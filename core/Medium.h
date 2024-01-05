#pragma	once

#include "Utils.h"
#include "Spectrum.h"
#include "Sampling.h"
#include "Sampler.h"

enum PhaseFunctionType {
	IsotropicPhaseFunction,
	HenyeyGreensteinPhaseFunction
};

class PhaseFunction {
public:
	PhaseFunction(PhaseFunctionType type) : m_type(type) {}

	inline PhaseFunctionType GetType() const {
		return m_type;
	}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) = 0;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

protected:
	PhaseFunctionType m_type;
};

class Isotropic : public PhaseFunction {
public:
	Isotropic() : PhaseFunction(PhaseFunctionType::IsotropicPhaseFunction) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;
};

class HenyeyGreenstein : public PhaseFunction {
public:
	HenyeyGreenstein(const RGBSpectrum& gg) : PhaseFunction(PhaseFunctionType::HenyeyGreensteinPhaseFunction), g(gg) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	RGBSpectrum g;
};

enum MediumType {
	HomogeneousMedium
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

	virtual RGBSpectrum EvaluateDistance(bool scattered, float distance, float& trans_pdf) = 0;

	virtual RGBSpectrum SampleDistance(float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) = 0;

protected:
	MediumType m_type;
	std::shared_ptr<PhaseFunction> phaseFunction;
};

class Homogeneous : public Medium {
public:
	Homogeneous(std::shared_ptr<PhaseFunction> phase, const RGBSpectrum& s, const RGBSpectrum& a);

	virtual RGBSpectrum EvaluateDistance(bool scattered, float distance, float& trans_pdf) override;

	virtual RGBSpectrum SampleDistance(float max_distance, float& distance, float& trans_pdf, bool& scattered, std::shared_ptr<Sampler> sampler) override;

private:
	float medium_sampling_weight;
	RGBSpectrum sigma_s;
	RGBSpectrum sigma_t;
};