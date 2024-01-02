#include "Camera.h"

Pinhole::Pinhole(const Point3f& lookfrom, const Point3f& lookat, const Vector3f& vup, float znear, float vfov, float aspect) 
	: Camera(CameraType::PinholeCamera) {
	origin = lookfrom;

	float theta = glm::radians(vfov);

	float half_height = std::abs(znear) * std::tan(theta / 2.0f);
	float half_width = aspect * half_height;
	w = glm::normalize(lookfrom - lookat);// z
	u = glm::normalize(glm::cross(vup, w));// x
	v = glm::normalize(glm::cross(w, u));// y

	lower_left_corner = origin - half_width * u - half_height * v - std::abs(znear) * w;

	horizontal = 2.0f * half_width * u;
	vertical = 2.0f * half_height * v;
}

Ray Pinhole::GenerateRay(std::shared_ptr<Sampler> sampler, float x, float y) {
	Vector3f direction = glm::normalize(lower_left_corner + x * horizontal + y * vertical - origin);

	return Ray(origin, direction);
}

Thinlens::Thinlens(const Point3f& lookfrom, const Point3f& lookat, const Vector3f& vup, float znear, float vfov, float aspect, float aperture)
	: Camera(CameraType::ThinlensCamera) {
	origin = lookfrom;
	float focus_dist = glm::length(lookfrom - lookat);

	lens_radius = aperture / 2.0f;
	float theta = glm::radians(vfov);

	float half_height = std::abs(znear) * std::tan(theta / 2.0f);
	float half_width = aspect * half_height;
	w = glm::normalize(lookfrom - lookat);
	u = glm::normalize(glm::cross(vup, w));
	v = glm::normalize(glm::cross(w, u));

	lower_left_corner = origin - half_width * u * focus_dist - half_height * v * focus_dist - std::abs(znear) * w * focus_dist;

	horizontal = 2.0f * half_width * u * focus_dist;
	vertical = 2.0f * half_height * v * focus_dist;
}

Ray Thinlens::GenerateRay(std::shared_ptr<Sampler> sampler, float x, float y) {
	Point2f rd = UniformSampleDisk(sampler->Get2(), lens_radius);
	Point3f offset = u * rd.x + v * rd.y;
	offset.z = 0.0f;

	Vector3f direction = glm::normalize(lower_left_corner + x * horizontal + y * vertical - origin - offset);

	return Ray(origin + offset, direction);
}