#pragma once

#include <Utils.h>
#include <Shape.h>
#include <Light.h>
#include <Camera.h>
#include <Filter.h>
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

class Scene {
public:
	Scene(RTCDevice& device);
	~Scene();

	void AddShape(Shape* shape);
	void AddLight(shared_ptr<Light> light, Shape* shape);
	void SetCamera(shared_ptr<Camera> c);
	void SetFilter(shared_ptr<Filter> f);
	void SetHDR(shared_ptr<InfiniteAreaLight> e);
	void Commit();
	void Intersect(RTCRayHit& rayhit);
	bool IsVisibility(RTCRayHit& shadowRayHit);
	void UpdateInfo(const RTCRayHit& rayhit, IntersectionInfo& info);

private:
	void FlipNormal(const RTCRayHit& rayhit, IntersectionInfo& info);

public:
	RTCDevice rtc_device;
	RTCScene rtc_scene;
	RTCIntersectContext context;
	int width, height;
	int depth;
	vector<Shape*> shapes;
	vector<shared_ptr<Light>> lights;
	shared_ptr<Camera> camera;
	shared_ptr<Filter> filter;
	shared_ptr<InfiniteAreaLight> env;
	bool useEnv;
};
