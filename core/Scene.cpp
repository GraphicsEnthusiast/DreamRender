#include "Scene.h"

Scene::Scene(const RTCDevice& device) {
	// Creating a new device
	rtc_device = device;

	// Creating a new scene
	rtc_scene = rtcNewScene(rtc_device);
	rtcSetSceneFlags(rtc_scene, RTC_SCENE_FLAG_COMPACT | RTC_SCENE_FLAG_ROBUST);

	rtcInitIntersectContext(&context);
}

Scene::~Scene() {
	for (auto& shape : shapes) {
		if (shape != NULL) {
			delete shape;
			shape = NULL;
		}
	}

	rtcReleaseScene(rtc_scene);
	rtcReleaseDevice(rtc_device);
}

void Scene::AddShape(Shape* shape) {
	shapes.push_back(shape);
}

void Scene::SetCamera(std::shared_ptr<Camera> c) {
	camera = c;
}

void Scene::Commit() {
	// Constructing Embree objects, setting VBOs/IBOs
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->ConstructEmbreeObject(rtc_device, rtc_scene);
	}

	// Loading the scene
	rtcCommitScene(rtc_scene);
}

void Scene::Intersect(RTCRayHit& rayhit) {
	rtcIntersect1(rtc_scene, &context, &rayhit);
}

void Scene::ClosestHit(const RTCRayHit& rayhit, IntersectionInfo& info) {
	int id = rayhit.hit.geomID;
	Vector3f V = -GetRayDir(rayhit);

	if (shapes[id]->GetType() == ShapeType::TriangleMeshShape) {
		Shape* shape = shapes[id];
		info.uv = shape->GetTexcoords(rayhit.hit.primID, Point2f(rayhit.hit.u, rayhit.hit.v));
		Vector3f Ns = shape->GetShadeNormal(rayhit.hit.primID, Point2f(rayhit.hit.u, rayhit.hit.v));
		Vector3f Ng = shape->GetGeometryNormal(rayhit.hit.primID, Point2f(rayhit.hit.u, rayhit.hit.v));
		info.SetNormal(V, Ng, Ns);
	}
	else {
		info.uv = Point2f(rayhit.hit.u, rayhit.hit.v);
		Vector3f N = glm::normalize(Vector3f(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z));
		info.SetNormal(V, N, N);
	}

	info.t = rayhit.ray.tfar;
	info.position = GetHitPos(rayhit);
	info.material = shapes[id]->GetMaterial();
}

void Scene::Miss(const RTCRayHit& rayhit, IntersectionInfo& info) {
	info.t = Infinity;
	info.frontFace = true;
	info.Ng = Vector3f(0.0f);
	info.Ns = Vector3f(0.0f);
	info.position = Point3f(Infinity);
	info.uv = Point2f(0.0f);
	info.material = NULL;
}

void Scene::TraceRay(RTCRayHit& rayhit, IntersectionInfo& info) {
	Intersect(rayhit);
	if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		ClosestHit(rayhit, info);
	}
	else {
		Miss(rayhit, info);
	}
}