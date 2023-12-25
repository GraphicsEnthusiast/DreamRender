#pragma once

#include "Utils.h"
#include "Transform.h"

class Camera {
public:
	Camera(Transform cameraToWorld, float width, float height, float hFov, float _nearclip = 1.0f, float _farclip = 10000.0f);

	virtual RTCRay GenerateRay(float x, float y) = 0;

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

class PinholeCamera : public Camera {
public:
	PinholeCamera(Transform cameraToWorld, float width, float height, float hFov, float _nearclip = 1.0f, float _farclip = 10000.0f) :
		Camera(cameraToWorld, width, height, hFov, _nearclip, _farclip) {}

	virtual RTCRay GenerateRay(float x, float y) override;
};

class ThinlensCamera : public Camera {
public:
	ThinlensCamera(Transform cameraToWorld, float width, float height, float hFov, float _nearclip = 1.0f, float _farclip = 10000.0f) :
		Camera(cameraToWorld, width, height, hFov, _nearclip, _farclip) {}

	virtual RTCRay GenerateRay(float x, float y) override;
};