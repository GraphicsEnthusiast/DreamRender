#include "Camera.h"

Camera::Camera(CameraType type, Transform cameraToWorld, float width, float height, float hFov, float nearclip, float farclip) : 
	m_type(type), nearClip(nearclip), farClip(farclip) {
	float aspect = float(width) / float(height);
	lensRadius = 0.000025f;

	cameraToWorld = cameraToWorld;
	worldToCamera = cameraToWorld.Inverse();

	rasterToCamera = (Transform::Scale(width, height, 1.0f) *
		Transform::Scale(-0.5f, 0.5f * aspect, 1.0f) *
		Transform::Translate(-1.0f, 1.0f / aspect, 0.0f) *
		Transform::Perspective(hFov, nearClip, farClip)).Inverse();
	cameraToRaster = rasterToCamera.Inverse();

	origin = cameraToWorld.TransformPoint(Point3f(0.0f));
	front = glm::normalize(cameraToWorld.TransformPoint(Vector3f(0.0f, 0.0f, 1.0f)) - cameraToWorld.TransformPoint(Vector3f(0.0f)));
}

Ray Pinhole::GenerateRay(Sampler* sampler, float x, float y) {
	// Camera space
	Point3f pOrigin(0.0f);

	Point3f pTarget = rasterToCamera.TransformPoint(Point3f(x, y, 0.0f));
	Vector3f rayDir = glm::normalize(pTarget - pOrigin);

	// Convert to world space
	Point3f pOriginWorld = cameraToWorld.TransformPoint(pOrigin);
	Vector3f pDirWorld = glm::normalize(cameraToWorld.TransformVector(rayDir));

	return Ray(pOriginWorld, pDirWorld);
}

Ray Thinlens::GenerateRay(Sampler* sampler, float x, float y) {
	// Camera space
	Point3f pOrigin(0.0f);

	if (lensRadius > 0.0f) {
		Vector2f diskSample = UniformSampleDisk(sampler->Get2(), lensRadius);
		pOrigin = Point3f(diskSample.x, diskSample.y, 0.0f);
	}

	Point3f pTarget = rasterToCamera.TransformPoint(Point3f(x, y, 0.0f));
	Vector3f rayDir = glm::normalize(pTarget - pOrigin);

	// Convert to world space
	Point3f pOriginWorld = cameraToWorld.TransformPoint(pOrigin);
	Vector3f pDirWorld = glm::normalize(cameraToWorld.TransformVector(rayDir));

	return Ray(pOriginWorld, pDirWorld);
}
