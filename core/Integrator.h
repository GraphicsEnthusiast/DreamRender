#pragma once

#include "Utils.h"
#include "Scene.h"
#include "Sampler.h"
#include "Filter.h"
#include "Medium.h"
#include "Spectrum.h"
#include "PostProcessing.h"

enum IntegratorType {
	VolumetricPathTracingIntegrator
};

class Integrator {
	friend Renderer;

public:
	Integrator(IntegratorType type, std::shared_ptr<Scene> s, std::shared_ptr<Sampler> sa, std::shared_ptr<Filter> f, int w, int h) :
		m_type(type), scene(s), sampler(sa), filter(f), width(w), height(h) {}

	inline IntegratorType GetType() const {
		return m_type;
	}

	float PowerHeuristic(float pdf1, float pdf2, int beta);

	virtual Spectrum SolvingIntegrator(Ray& ray, IntersectionInfo& info) = 0;

	virtual void RenderImage(const PostProcessing& post, Spectrum* image) = 0;

protected:
	IntegratorType m_type;
	int width, height;
	std::shared_ptr<Scene> scene;
	std::shared_ptr<Filter> filter;
	std::shared_ptr<Sampler> sampler;
};

class VolumetricPathTracing : public Integrator {
public:
	VolumetricPathTracing(std::shared_ptr<Scene> s, std::shared_ptr<Sampler> sa, std::shared_ptr<Filter> f, int w, int h, int bounce) :
		Integrator(IntegratorType::VolumetricPathTracingIntegrator, s, sa, f, w, h), maxBounce(bounce) {}

	virtual Spectrum SolvingIntegrator(Ray& ray, IntersectionInfo& info) override;

	virtual void RenderImage(const PostProcessing& post, Spectrum* image) override;

private:
	int maxBounce;
};