#pragma once

#include "Utils.h"
#include "Ray.h"
#include "Transform.h"
#include "Sampling.h"
#include "Sampler.h"

enum CameraType {
	PinholeCamera,
	ThinlensCamera
};

class Camera {
public:
	Camera(CameraType type, Transform cameraToWorld, float width, float height, float hFov, float nearclip = 1.0f, float farclip = 10000.0f);

	virtual Ray GenerateRay(Sampler* sampler, float x, float y) = 0;

	inline virtual CameraType GetType() const {
		return m_type;
	}

protected:
	CameraType m_type;
	Point3f origin;
	Vector3f front;
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
		Camera(CameraType::PinholeCamera, cameraToWorld, width, height, hFov, _nearclip, _farclip) {}

	virtual Ray GenerateRay(Sampler* sampler, float x, float y) override;
};

class Thinlens : public Camera {
public:
	Thinlens(Transform cameraToWorld, float width, float height, float hFov, float _nearclip = 1.0f, float _farclip = 10000.0f) :
		Camera(CameraType::ThinlensCamera, cameraToWorld, width, height, hFov, _nearclip, _farclip) {}

	virtual Ray GenerateRay(Sampler* sampler, float x, float y) override;
};