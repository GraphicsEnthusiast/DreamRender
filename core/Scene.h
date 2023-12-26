#pragma once

#include "Utils.h"
#include "Shape.h"
#include "Camera.h"

class Scene {
public:
	Scene(const RTCDevice& device);

	~Scene();

	void AddShape(Shape* shape);

	void SetCamera(std::shared_ptr<Camera> c);

	void Commit();

	bool IsVisibility(RTCRayHit& shadowRayHit);

	void TraceRay(RTCRayHit& rayhit, IntersectionInfo& info);

private:
	void Intersect(RTCRayHit& rayhit);

	void ClosestHitRadiance(const RTCRayHit& rayhit, IntersectionInfo& info);

	void MissRadiance(const RTCRayHit& rayhit, IntersectionInfo& info);

private:
	RTCDevice rtc_device;
	RTCScene rtc_scene;
	RTCIntersectContext context;
	std::vector<Shape*> shapes;
	std::shared_ptr<Camera> camera;
};