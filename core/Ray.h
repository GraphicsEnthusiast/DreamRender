#pragma once

#include "Utils.h"

// inline constexpr float Origin() {
// 	return 1.0f / 32.0f;
// }
// 
// inline constexpr float FloatScale() {
// 	return 1.0f / 65536.0f;
// }
// 
// inline constexpr float IntScale() {
// 	return 256.0f;
// }
// 
// // Normal points outward for rays exiting the surface, else is flipped.
// inline Point3f OffsetRay(const Point3f& p, const Vector3f& n) {
// 	Point3i of_i(IntScale() * n.x, IntScale() * n.y, IntScale() * n.z);
// 
// 	Point3f p_i(
// 		float(int(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
// 		float(int(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
// 		float(int(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));
// 
// 	return Point3f(std::abs(p.x) < Origin() ? p.x + FloatScale() * n.x : p_i.x,
// 		std::abs(p.y) < Origin() ? p.y + FloatScale() * n.y : p_i.y,
// 		std::abs(p.z) < Origin() ? p.z + FloatScale() * n.z : p_i.z);
// }

class Ray {
public:
	Ray(const Point3f& ori, const Vector3f& dir) : origin(ori), direction(dir) {}

	inline Point3f GetOrg() const {
		return origin;
	}

	inline Vector3f GetDir() const {
		return direction;
	}

private:
	static Point3f OffsetRayOrigin(const Point3f& p, const Vector3f& pError, const Vector3f& N, const Vector3f& L);

public:
	static Ray SpawnRay(const Point3f& pos, const Vector3f& L, const Vector3f& Ng);

private:
	Point3f origin;
	Vector3f direction;
};

inline RTCRay RayToRTCRay(const Ray& ray) {
	return MakeRay(ray.GetOrg(), ray.GetDir());
}

inline Ray RTCRayToRay(const RTCRay& rtc_ray) {
	Ray ray(GetRayOrg(rtc_ray), GetRayDir(rtc_ray));

	return ray;
}