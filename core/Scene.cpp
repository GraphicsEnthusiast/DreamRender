#include "Scene.h"

Scene::Scene(const RTCDevice& device) {
	infiniteLight = NULL;

	// Creating a new device
	rtc_device = device;

	// Creating a new scene
	rtc_scene = rtcNewScene(rtc_device);
	rtcSetSceneFlags(rtc_scene, RTC_SCENE_FLAG_COMPACT | RTC_SCENE_FLAG_ROBUST);

	rtcInitIntersectContext(&context);
}

Scene::~Scene() {
	rtcReleaseScene(rtc_scene);
	rtcReleaseDevice(rtc_device);
}

void Scene::AddShape(Shape* shape) {
	shapes.push_back(shape);
}

void Scene::AddLight(std::shared_ptr<Light> light) {
	if (light->GetType() == LightType::InfiniteAreaLight) {
		infiniteLight = light;
	}
	else {
		shapes.push_back(light->GetShape());
		shapeToLight.insert({ light->GetShape()->GetGeometryID(), lights.size() });
	}
	lights.push_back(light);
}

void Scene::SetCamera(std::shared_ptr<Camera> c) {
	camera = c;
}

std::shared_ptr<Camera> Scene::GetCamera() const {
	return camera;
}

void Scene::Commit() {
	std::vector<float> power(lights.size());
	for (int i = 0; i < lights.size(); i++) {
		auto light = lights[i];
		float pdf = light->LightLuminance();
		power[i] = pdf;
	}
	lightTable = AliasTable1D(power);

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
	Vector3f dir = GetRayDir(rayhit);

	if (shapes[id]->GetType() == ShapeType::TriangleMeshShape) {
		Shape* shape = shapes[id];
		info.uv = shape->GetTexcoords(rayhit.hit.primID, Point2f(rayhit.hit.u, rayhit.hit.v));
		Vector3f Ns = shape->GetShadeNormal(rayhit.hit.primID, Point2f(rayhit.hit.u, rayhit.hit.v));
		Vector3f Ng = shape->GetGeometryNormal(rayhit.hit.primID, Point2f(rayhit.hit.u, rayhit.hit.v));
		info.SetNormal(dir, Ng, Ns);
	}
	else {
		info.uv = Point2f(rayhit.hit.u, rayhit.hit.v);
		Vector3f N = glm::normalize(Vector3f(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z));
		info.SetNormal(dir, N, N);
	}

	info.t = rayhit.ray.tfar;
	info.position = GetHitPos(rayhit);
	info.material = shapes[id]->GetMaterial();
	info.geomID = id;
}

void Scene::Miss(const RTCRayHit& rayhit, IntersectionInfo& info) {
	info.t = Infinity;
	info.frontFace = true;
	info.Ng = Vector3f(0.0f);
	info.Ns = Vector3f(0.0f);
	info.position = Point3f(Infinity);
	info.uv = Point2f(0.0f);
	info.material = NULL;
	info.geomID = -1;
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

RGBSpectrum Scene::SampleLightByPower(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	if (lights.size() == 0) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	int index = lightTable.Sample(sampler->Get2());
	auto light = lights[index];
	float dist = 0.0f;
	RGBSpectrum radiance = light->Sample(L, pdf, dist, info, sampler);
	pdf *= (light->LightLuminance() / lightTable.Sum());

	Ray ray(info.position, L);
	RTCRayHit rtc_rayhit = MakeRayHit(ray.GetOrg(), ray.GetDir(), ShadowEpsilon, dist - ShadowEpsilon);

	Intersect(rtc_rayhit);
	if (rtc_rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		return RGBSpectrum(0.0f);
	}

	return radiance;
}

RGBSpectrum Scene::EvaluateLight(int geomID, const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	int lightSize = infiniteLight == NULL ? lights.size() : lights.size() - 1;
	if (lightSize == 0) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	int index = shapeToLight[geomID];
	auto light = lights[index];
	RGBSpectrum radiance = light->Evaluate(L, pdf, info);
	pdf *= (light->LightLuminance() / lightTable.Sum());

	return radiance;
}

RGBSpectrum Scene::EvaluateEnvironment(const Vector3f& L, float& pdf) {
	if (infiniteLight == NULL) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	RGBSpectrum radiance = infiniteLight->EvaluateEnvironment(L, pdf);
	pdf *= (infiniteLight->LightLuminance() / lightTable.Sum());

	return radiance;
}
