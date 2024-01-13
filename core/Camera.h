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

struct CameraParams {
	CameraType type;
	Point3f lookfrom;
	Point3f lookat;
	Vector3f vup;
	float znear;
	float vfov;
	float aspect;
	float aperture;
	std::shared_ptr<Medium> medium;
};

class Camera {
public:
	Camera(CameraType type, std::shared_ptr<Medium> med = NULL) : m_type(type), medium(med) {}

	virtual Ray GenerateRay(std::shared_ptr<Sampler> sampler, float x, float y) = 0;

	inline CameraType GetType() const {
		return m_type;
	}
	
	inline std::shared_ptr<Medium> GetMedium() const {
		return medium;
	}

	static std::shared_ptr<Camera> Create(const CameraParams& params);

protected:
	CameraType m_type;
	Point3f origin;
	Point3f lower_left_corner;
	Vector3f horizontal;
	Vector3f vertical;
	Vector3f u, v, w;
	std::shared_ptr<Medium> medium;
};

class Pinhole : public Camera {
public:
	Pinhole(const Point3f& lookfrom, const Point3f& lookat, const Vector3f& vup, float znear, float vfov, float aspect, std::shared_ptr<Medium> med = NULL);

	virtual Ray GenerateRay(std::shared_ptr<Sampler> sampler, float x, float y) override;
};

class Thinlens : public Camera {
public:
	Thinlens(const Point3f& lookfrom, const Point3f& lookat, const Vector3f& vup, float znear, float vfov, float aspect, float aperture, std::shared_ptr<Medium> med = NULL);

	virtual Ray GenerateRay(std::shared_ptr<Sampler> sampler, float x, float y) override;

private:
	float lens_radius;
};