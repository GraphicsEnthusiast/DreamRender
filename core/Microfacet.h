#pragma once

#include "Utils.h"
#include "Spectrum.h"

namespace GGX {
	float GeometrySmith1(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	float Distribution(const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	float DistributionVisible(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	Vector3f SampleVisible(const Vector3f& N, const Vector3f& V, float alpha_u, float alpha_v, const Point2f& sample);
}