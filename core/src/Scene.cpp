#include <Scene.h>

Scene::Scene(RTCDevice& device) {
	useEnv = false;

	// Creating a new device
	rtc_device = device;

	// Creating a new scene
	rtc_scene = rtcNewScene(rtc_device);
	rtcSetSceneFlags(rtc_scene, RTC_SCENE_FLAG_COMPACT | RTC_SCENE_FLAG_ROBUST);

	rtcInitIntersectContext(&context);
}

Scene::~Scene() {
	lights.clear();
	shapes.clear();
	rtcReleaseScene(rtc_scene);
	rtcReleaseDevice(rtc_device);
}

void Scene::AddShape(Shape* shape) {
	shapes.push_back(shape);
}

void Scene::AddLight(shared_ptr<Light> light, Shape* shape) {
	lights.push_back(light);
	if (shape != NULL) {
		shapes.push_back(shape);
	}
}

void Scene::SetCamera(shared_ptr<Camera> c) {
	camera = c;
}

void Scene::SetFilter(shared_ptr<Filter> f) {
	filter = f;
}

void Scene::SetHDR(shared_ptr<InfiniteAreaLight> e) {
	env = e;
	useEnv = true;
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

bool Scene::IsVisibility(RTCRayHit& shadowRayHit) {
	rtcIntersect1(rtc_scene, &context, &shadowRayHit);
	if (shadowRayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		return false;
	}
	return true;
}

void Scene::FlipNormal(const RTCRayHit& rayhit, IntersectionInfo& info) {
	vec3 normal;
	int id = rayhit.hit.geomID;
	vec3 dir = GetRayDir(rayhit);

	if (shapes[id]->m_type == ShapeType::TriangleMesh) {
		normal = shapes[id]->GetFaceNormal(rayhit.hit.primID, vec2(rayhit.hit.u, rayhit.hit.v));
	}
	else {
		normal = normalize(vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z));
	}

	info.normal = normal;
	info.SetFaceNormal(GetRayDir(rayhit), info.normal);
}

void Scene::UpdateInfo(const RTCRayHit& rayhit, IntersectionInfo& info) {
	FlipNormal(rayhit, info);
	int id = rayhit.hit.geomID;
	if (shapes[id]->m_type == ShapeType::TriangleMesh) {
		info.uv = shapes[id]->GetTexcoords(rayhit.hit.primID, vec2(rayhit.hit.u, rayhit.hit.v));
	}
	else {
		info.uv = vec2(rayhit.hit.u, rayhit.hit.v);
	}
	info.t = rayhit.ray.tfar;
	info.position = GetHitPos(rayhit);
}
