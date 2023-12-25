#pragma once

#include "Utils.h"
#include "Ray.h"
#include "Transform.h"

enum CameraType {
	PinholeCamera,
	ThinlensCamera
};

class Camera {
public:
	Camera(Transform cameraToWorld, float width, float height, float hFov, float nearclip = 1.0f, float farclip = 10000.0f);

	virtual Ray GenerateRay(float x, float y) = 0;

protected:
	Point3f origin;
	float nearClip;
	float farClip;
	float lensRadius;
	Transform cameraToWorld;
	Transform worldToCamera;
	Transform rasterToCamera;
	Transform cameraToRaster;
};

class Pinhole : public Camera {
public:
	Pinhole(Transform cameraToWorld, float width, float height, float hFov, float _nearclip = 1.0f, float _farclip = 10000.0f) :
		Camera(cameraToWorld, width, height, hFov, _nearclip, _farclip) {}

	virtual Ray GenerateRay(float x, float y) override;
};

class Thinlens : public Camera {
public:
	Thinlens(Transform cameraToWorld, float width, float height, float hFov, float _nearclip = 1.0f, float _farclip = 10000.0f) :
		Camera(cameraToWorld, width, height, hFov, _nearclip, _farclip) {}

	virtual Ray GenerateRay(float x, float y) override;
};