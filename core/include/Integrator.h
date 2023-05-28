#pragma once

#include <Utils.h>
#include <Scene.h>
#include <Sampler.h>
#include <Material.h>

enum TraceLightType {
	RANDOM,
	ALL
};

struct GBuffer {
	vec3* albedoTexture;
	vec3* normalTexture;
};

class Integrator {
public:
	Integrator(shared_ptr<Scene> s, SamplerType sp) : scene(s), sp_type(sp) {}

	bool ReasonableTesting(float value);
	GBuffer GetSceneGBuffer();
	float PowerHeuristic(float pdf1, float pdf2, int beta);

	virtual vec3 SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) = 0;
	virtual vec3 GetPixelColor(IntersectionInfo& info);

public:
	shared_ptr<Scene> scene;
	Sampler* sampler;
	SamplerType sp_type;
};

//揃抄弖忸！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
class PathTracing : public Integrator {
public:
	PathTracing(shared_ptr<Scene> s, SamplerType sp, TraceLightType t = RANDOM) : Integrator(s, sp), traceLightType(t), depth(scene->depth) {}

	vec3 DirectLight(const RTCRayHit& rayhit, const IntersectionInfo& info, const vec3& history);

	virtual vec3 SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) override;

private:
	int depth;
	TraceLightType traceLightType;
};