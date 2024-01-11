#pragma once

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

	virtual Spectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) = 0;

	virtual Spectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

protected:
	PhaseFunctionType m_type;
};

class Isotropic : public PhaseFunction {
public:
	Isotropic() : PhaseFunction(PhaseFunctionType::IsotropicPhaseFunction) {}

	virtual Spectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual Spectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;
};

class HenyeyGreenstein : public PhaseFunction {
public:
	HenyeyGreenstein(const Spectrum& gg) : PhaseFunction(PhaseFunctionType::HenyeyGreensteinPhaseFunction), g(gg) {}

	virtual Spectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual Spectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	Spectrum g;
};