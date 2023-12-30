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
	Camera(CameraType type) : m_type(type) {}

	virtual Ray GenerateRay(Sampler* sampler, float x, float y) = 0;

	virtual inline CameraType GetType() const {
		return m_type;
	}

protected:
	CameraType m_type;
	Point3f origin;
	Point3f lower_left_corner;
	Vector3f horizontal;
	Vector3f vertical;
	Vector3f u, v, w;
};

class Pinhole : public Camera {
public:
	Pinhole(const Point3f& lookfrom, const Point3f& lookat, const Vector3f& vup, float znear, float vfov, float aspect);

	virtual Ray GenerateRay(Sampler* sampler, float x, float y) override;
};

class Thinlens : public Camera {
public:
	Thinlens(const Point3f& lookfrom, const Point3f& lookat, const Vector3f& vup, float znear, float vfov, float aspect, float aperture);

	virtual Ray GenerateRay(Sampler* sampler, float x, float y) override;

private:
	float lens_radius;
};