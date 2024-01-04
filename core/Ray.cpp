#include "Ray.h"

Point3f Ray::OffsetRayOrigin(const Point3f& p, const Vector3f& pError, const Vector3f& N, const Vector3f& L) {
	float d = glm::dot(glm::abs(N), pError);
	Vector3f offset = d * Vector3f(N);
	if (glm::dot(L, N) < 0.0f) {
		offset = -offset;
	}
	Point3f po = p + offset;
	// Round offset point _po_ away from _p_
	for (int i = 0; i < 3; ++i) {
		if (offset[i] > 0) {
			po[i] = NextFloatUp(po[i]);
		}
		else if (offset[i] < 0) {
			po[i] = NextFloatDown(po[i]);
		}
	}

	return po;
}

Ray Ray::SpawnRay(const Point3f& pos, const Vector3f& L, const Vector3f& Ng) {
	Vector3f N = Ng;
	if (glm::dot(L, Ng) < 0.0f) {
		N = -Ng;
	}

	Ray ray(OffsetRayOrigin(pos, Vector3f(Epsilon), N, L), L);

	return ray;
}
