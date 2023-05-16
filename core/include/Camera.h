#pragma once

#include <Utils.h>

class Camera {
public:
	virtual auto GenerateRay(float x, float y) const noexcept -> RTCRay = 0;

public:
	vec3 origin;
};

class PinholeCamera : public Camera {
public:
	PinholeCamera(vec3 lookfrom, vec3 lookat, vec3 vup, float znear, float vfov, float aspect);

	virtual auto GenerateRay(float x, float y) const noexcept -> RTCRay override;

public:
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
	vec3 u, v, w;//x, y, z
};