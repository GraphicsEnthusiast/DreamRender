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
		float inter_normal[] = { 0.0f, 0.0f, 0.0f };
		rtcInterpolate0(rtcGetGeometry(rtc_scene, id), rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, inter_normal, 3);
		normal = vec3(inter_normal[0], inter_normal[1], inter_normal[2]);

// 		vec3 dPdu, dPdv;
// 		rtcInterpolate1(rtcGetGeometry(rtc_scene, id), rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v, RTC_BUFFER_TYPE_VERTEX, 0, nullptr, &dPdu.x, &dPdv.x, 3);
// 		normal = normalize(cross(dPdu, dPdv));

// 		cout << endl;
// 		cout << inter_normal[0] << " " << inter_normal[1] << " " << inter_normal[2] << endl;
// 		cout << ffnormal.x << " " << ffnormal.y << " " << ffnormal.z << endl;
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
		float inter_uv[] = { 0.0f, 0.0f };
		rtcInterpolate0(rtcGetGeometry(rtc_scene, id), rayhit.hit.primID, rayhit.hit.u, rayhit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, inter_uv, 2);
		info.uv = vec2(inter_uv[0], inter_uv[1]);
//		cout << to_string(info.uv) << endl;
	}
	else {
		info.uv = vec2(rayhit.hit.u, rayhit.hit.v);
	}
	info.t = rayhit.ray.tfar;
	info.position = GetHitPos(rayhit);
// 	info.uv = vec2(rayhit.hit.u, rayhit.hit.v);
// 	if (shapes[id]->m_type == ShapeType::Quad) {
// 		info.uv = Quad::GetQuadUV(info.position, (Quad*)shapes[id]);
// 	}
// 	else {
// 		info.uv = vec2(rayhit.hit.u, rayhit.hit.v);
// 	}
}
