#pragma once

#include "Utils.h"
#include "Shape.h"
#include "Camera.h"
#include "Light.h"

class Scene {
public:
	Scene(const RTCDevice& device);

	~Scene();

	void AddShape(Shape* shape);

	void AddLight(std::shared_ptr<Light> light);

	void SetCamera(std::shared_ptr<Camera> c);

	std::shared_ptr<Camera> GetCamera() const;

	void Commit();

	void TraceRay(RTCRayHit& rayhit, IntersectionInfo& info);

	RGBSpectrum SampleLightByPower(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler);

	RGBSpectrum EvaluateLightByPower(int geomID, const Vector3f&L, float& pdf, const IntersectionInfo& info);

private:
	void Intersect(RTCRayHit& rayhit);

	void ClosestHit(const RTCRayHit& rayhit, IntersectionInfo& info);

	void Miss(const RTCRayHit& rayhit, IntersectionInfo& info);

private:
	RTCDevice rtc_device;
	RTCScene rtc_scene;
	RTCIntersectContext context;
	std::vector<Shape*> shapes;
	std::vector<std::shared_ptr<Light>> lights;
	std::vector<std::shared_ptr<Light>> infinityLights;
	std::shared_ptr<Camera> camera;
	std::map<int, int> shapeToLight;
	AliasTable1D lightTable;
};