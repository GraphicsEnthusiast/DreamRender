#pragma once

#include <Utils.h>
#include <Medium.h>

class Camera {
public:
	Camera(shared_ptr<Medium> m) : medium(m) {}

	virtual RTCRay GenerateRay(float x, float y) noexcept = 0;

public:
	vec3 origin;
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
	vec3 u, v, w;//x, y, z
	shared_ptr<Medium> medium;
};

class PinholeCamera : public Camera {
public:
	PinholeCamera(vec3 lookfrom, vec3 lookat, vec3 vup, float znear, float vfov, float aspect, shared_ptr<Medium> m = NULL);

	virtual RTCRay GenerateRay(float x, float y) noexcept override;
};

class ThinlensCamera : public Camera {
public:
	ThinlensCamera(vec3 lookfrom, vec3 lookat, vec3 vup, float znear, float vfov, float aspect, float aperture, float focus_dist, shared_ptr<Medium> m = NULL);

	inline float RandomFloat() {
		return static_cast<float>(rand() / (RAND_MAX + 1.0f));
	}

	inline vec3 RandomInUnitDisk() {
		while (true) {
			float x = 2.0f * RandomFloat() - 1.0f;
			float y = 2.0f * RandomFloat() - 1.0f;
			if (x * x + y * y < 1.0f) {
				return vec3(x, y, 0.0f);
			}
		}
	}

	virtual RTCRay GenerateRay(float x, float y) noexcept override;

public:
	float lens_radius;
};