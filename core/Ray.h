#pragma once

#include "Utils.h"

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