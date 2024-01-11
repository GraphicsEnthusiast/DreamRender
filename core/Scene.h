#pragma once

#include "Utils.h"
#include "Shape.h"
#include "Camera.h"
#include "Light.h"
#include "Medium.h"

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

	RGBSpectrum SampleLightEnvironment(const RGBSpectrum& history, Vector3f& L, float& pdf, float& mult_trans_pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler);

	RGBSpectrum EvaluateLight(int geomID, const Vector3f& L, float& pdf, const IntersectionInfo& info);

	RGBSpectrum EvaluateEnvironment(const Vector3f& L, float& pdf);

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
	std::shared_ptr<Light> infiniteLight;
	std::shared_ptr<Camera> camera;
	std::map<int, int> shapeToLight;
	AliasTable1D lightTable;
};