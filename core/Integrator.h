#pragma once

#include "Utils.h"
#include "Scene.h"
#include "Sampler.h"
#include "Filter.h"
#include "Spectrum.h"

enum IntegratorType {
	VolumetricPathTracingIntegrator
};

class Integrator {
public:
	Integrator(IntegratorType type, std::shared_ptr<Scene> s, Sampler* sa, std::shared_ptr<Filter> f, int w, int h) :
		m_type(type), scene(s), sampler(sa), filter(f), width(w), height(h), image(new RGBSpectrum[width * height]) {}

	virtual ~Integrator();

	inline IntegratorType GetType() const {
		return m_type;
	}

	float PowerHeuristic(float pdf1, float pdf2, int beta);

	virtual RGBSpectrum SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) = 0;

	virtual RGBSpectrum* RenderImage() = 0;

protected:
	std::shared_ptr<Scene> scene;
	std::shared_ptr<Filter> filter;
	Sampler* sampler;
	int width, height;
	IntegratorType m_type;
	RGBSpectrum* image;
};

class VolumetricPathTracing : public Integrator {
public:
	VolumetricPathTracing(std::shared_ptr<Scene> s, Sampler* sa, std::shared_ptr<Filter> f, int w, int h, int bounce) : 
		Integrator(IntegratorType::VolumetricPathTracingIntegrator, s, sa, f, w, h), maxBounce(bounce) {}

	virtual RGBSpectrum SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) override;

	virtual RGBSpectrum* RenderImage() override;

private:
	int maxBounce;
};