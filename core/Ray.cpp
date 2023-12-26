#include "Ray.h"

Ray Ray::SpawnRay(const Point3f& pos, const Vector3f& L, const Vector3f& Ng) {
	Vector3f N = Ng;
	if (glm::dot(L, Ng) < 0.0f) {
		N = -Ng;
	}

	Ray ray(OffsetRay(pos, N), L);

	return ray;
}

Ray Ray::SpawnOcclusRay(const Point3f& pos, const Vector3f& L, const Vector3f& Ng) {
	Vector3f N = Ng;
	if (glm::dot(L, Ng) < 0.0f) {
		N = -Ng;
	}

	Ray ray(pos + N * ShadowEpsilon, L);

	return ray;
}
