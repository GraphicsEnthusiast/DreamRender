#include "Light.h"

Light::~Light() {
	if (shape != NULL) {
		delete shape;
		shape = NULL;
	}
}

RGBSpectrum Light::EvaluateEnvironment(const Vector3f& L, float& pdf) {
	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

RGBSpectrum QuadArea::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info, float distance) {
	Quad* quad = (Quad*)shape;
	Vector3f Nl = glm::cross(quad->u, quad->v);
	float cos_theta = glm::dot(L, Nl);
	if (!doubleSide && cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = glm::length(quad->u) * glm::length(quad->v);

	// surface pdf
	pdf = 1.0f / area;

	// solid angel pdf
	pdf *= glm::pow2(distance) / std::abs(cos_theta);

	return quad->material->Emit(info.uv);// info record a point on a light source
}

RGBSpectrum QuadArea::Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) {
	Quad* quad = (Quad*)shape;
	Vector3f Nl = glm::cross(quad->u, quad->v);
	Point3f pos = quad->position + quad->u * sampler->Get1() + quad->v * sampler->Get1();
	L = pos - info.position;
	float dist_sq = glm::dot(L, L);
	float distance = std::sqrt(dist_sq);
	L /= distance;

	float cos_theta = glm::dot(L, Nl);

	if (!doubleSide && cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = glm::length(quad->u) * glm::length(quad->v);
	pdf = 1.0f / area;
	pdf *= dist_sq / abs(cos_theta);

	Point2f uv = Quad::GetQuadUV(pos, quad->position, quad->u, quad->v);

	return quad->material->Emit(uv);// info record a point on a common shape
}
