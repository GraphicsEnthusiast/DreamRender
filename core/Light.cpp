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

RGBSpectrum QuadArea::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Quad* quad = (Quad*)shape;
	Vector3f Nl = glm::cross(quad->u, quad->v);
	float cos_theta = glm::dot(L, Nl);
	if (!twoSide && cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = glm::length(quad->u) * glm::length(quad->v);

	// surface pdf
	pdf = 1.0f / area;

	// solid angel pdf
	float distance = info.t;
	pdf *= glm::pow2(distance) / std::abs(cos_theta);

	return Radiance();// info record a point on a light source
}

RGBSpectrum QuadArea::Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Quad* quad = (Quad*)shape;
	Vector3f Nl = glm::cross(quad->u, quad->v);
	Point3f pos = quad->position + quad->u * sampler->Get1() + quad->v * sampler->Get1();
	L = pos - info.position;
	float dist_sq = glm::dot(L, L);
	float distance = std::sqrt(dist_sq);
	L /= distance;

	float cos_theta = glm::dot(L, Nl);

	if (!twoSide && cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = glm::length(quad->u) * glm::length(quad->v);
	pdf = 1.0f / area;
	pdf *= dist_sq / abs(cos_theta);

	return Radiance();// info record a point on a common shape
}

RGBSpectrum SphereArea::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Sphere* sphere = (Sphere*)shape;
	Vector3f dir = sphere->center - info.position;
	float dist_sq = glm::dot(dir, dir);
	float sin_theta_sq = sphere->radius * sphere->radius / dist_sq;
	float cos_theta = std::sqrt(1.0f - sin_theta_sq);
	pdf = UniformPdfCone(cos_theta);

	return Radiance();
}

RGBSpectrum SphereArea::Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Sphere* sphere = (Sphere*)shape;
	Vector3f dir = sphere->center - info.position;
	float dist_sq = dot(dir, dir);
	float inv_dist = 1.0f / sqrt(dist_sq);
	dir *= inv_dist;
	float distance = dist_sq * inv_dist;

	float sin_theta = sphere->radius * inv_dist;
	if (sin_theta < 1.0f) {
		float cos_theta = sqrt(1.0f - sin_theta * sin_theta);
		Vector3f local_L = UniformSampleCone(sampler->Get2(), cos_theta);
		float cos_i = local_L.z;
		L = ToWorld(local_L, dir);
		pdf = UniformPdfCone(cos_theta);

		return Radiance();
	}

	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}
