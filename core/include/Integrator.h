#pragma once

#include <Utils.h>
#include <Scene.h>
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
	Integrator(shared_ptr<Scene> s, TraceLightType t) : scene(s), traceLightType(t) {}

	bool ReasonableTesting(float value);
	GBuffer GetSceneGBuffer();
	float PowerHeuristic(float pdf1, float pdf2, int beta);

	virtual vec3 SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) = 0;
	virtual vec3 GetPixelColor(IntersectionInfo& info);

public:
	shared_ptr<Scene> scene;
	TraceLightType traceLightType;
};

//揃抄弖忸！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
class PathTracing : public Integrator {
public:
	PathTracing(shared_ptr<Scene> s, TraceLightType t = RANDOM) : Integrator(s, t), depth(scene->depth) {}

	vec3 DirectLight(const RTCRayHit& rayhit, const IntersectionInfo& info, const vec3& history);

	virtual vec3 SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) override;

private:
	int depth;
};