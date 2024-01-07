#include "Microfacet.h"

float GGX::GeometrySmith1(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v) {
	float cos_v_n = glm::dot(V, N);

	if (cos_v_n * glm::dot(V, H) <= 0.0f) {
		return 0.0f;
	}

	if (std::abs(cos_v_n - 1.0f) < Epsilon) {
		return 1.0f;
	}

	if (alpha_u == alpha_v) {
		float cos_v_n_2 = glm::pow2(cos_v_n),
			tan_v_n_2 = (1.0f - cos_v_n_2) / cos_v_n_2,
			alpha_2 = alpha_u * alpha_u;

		return 2.0f / (1.0f + std::sqrt(1.0f + alpha_2 * tan_v_n_2));
	}
	else {
		Vector3f dir = ToLocal(V, N);
		float xy_alpha_2 = glm::pow2(alpha_u * dir.x) + glm::pow2(alpha_v * dir.y),
			tan_v_n_alpha_2 = xy_alpha_2 / glm::pow2(dir.z);

		return 2.0f / (1.0f + std::sqrt(1.0f + tan_v_n_alpha_2));
	}
}

float GGX::Distribution(const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v) {
	float cos_theta = glm::dot(H, N);
	if (cos_theta <= 0.0f) {
		return 0.0f;
	}
	float cos_theta_2 = glm::pow2(cos_theta),
		tan_theta_2 = (1.0f - cos_theta_2) / cos_theta_2,
		alpha_2 = alpha_u * alpha_v;
	if (alpha_u == alpha_v) {
		return alpha_2 / (PI * glm::pow2(cos_theta) * glm::pow2(alpha_2 + tan_theta_2));
	}
	else {
		Vector3f dir = ToLocal(H, N);

		return 1.0f / (PI * alpha_2 * glm::pow2(glm::pow2(dir.x / alpha_u) + glm::pow2(dir.y / alpha_v) + glm::pow2(dir.z)));
	}
}

float GGX::DistributionVisible(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v) {
	return GeometrySmith1(V, H, N, alpha_u, alpha_v) * glm::dot(V, H) * Distribution(H, N, alpha_u, alpha_v) / glm::dot(N, V);
}

Vector3f GGX::SampleVisible(const Vector3f& N, const Vector3f& Ve, float alpha_u, float alpha_v, const Point2f& sample) {
	Vector3f V = ToLocal(Ve, N);
	Vector3f Vh = glm::normalize(Vector3f(alpha_u * V.x, alpha_v * V.y, V.z));

	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	float len2 = glm::pow2(Vh.x) + glm::pow2(Vh.y);
	Vector3f T1 = len2 > 0.0f ? Vector3f(-Vh.y, Vh.x, 0.0f) * glm::inversesqrt(len2) : Vector3f(1.0f, 0.0f, 0.0f);
	Vector3f T2 = glm::cross(Vh, T1);

	// Section 4.2: parameterization of the projected area
	float u = sample.x;
	float v = sample.y;
	float r = std::sqrt(u);
	float phi = v * 2.0f * PI;
	float t1 = r * std::cos(phi);
	float t2 = r * std::sin(phi);
	float s = 0.5f * (1.0f + Vh.z);
	t2 = (1.0f - s) * std::sqrt(1.0f - glm::pow2(t1)) + s * t2;

	// Section 4.3: reprojection onto hemisphere
	Vector3f Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0f, 1.0f - glm::pow2(t1) - glm::pow2(t2))) * Vh;

	// Section 3.4: transforming the normal back to the ellipsoid configuration
	Vector3f H = glm::normalize(Vector3f(alpha_u * Nh.x, alpha_v * Nh.y, std::max(0.0f, Nh.z)));

	return H;
}