#include "Filter.h"

Point2f Box::FilterPoint2f(const Point2f& sample) {
	return sample - Point2f(0.5f, 0.5f);
}

Point2f Tent::FilterPoint2f(const Point2f& sample) {
	Point2f j = sample;
	j = j * 2.0f;
	j.x = j.x < 1.0f ? sqrt(j.x) - 1.0f : 1.0f - sqrt(2.0f - j.x);
	j.y = j.y < 1.0f ? sqrt(j.y) - 1.0f : 1.0f - sqrt(2.0f - j.y);

	return Point2f(0.5f, 0.5f) + j;
}

Point2f Triangle::FilterPoint2f(const Point2f& sample) {
	float u1 = sample.x;
	float u2 = sample.y;
	if (u2 > u1) {
		u1 *= 0.5f;
		u2 -= u1;
	}
	else {
		u2 *= 0.5f;
		u1 -= u2;
	}

	return Point2f(0.5f, 0.5f) + Point2f(u1, u2);
}

Point2f Gaussian::FilterPoint2f(const Point2f& sample) {
	float r1 = std::max(FLT_MIN, sample.x);
	float r = sqrt(-2.0f * log(r1));
	float theta = 2.0f * PI * sample.y;
	Point2f uv = r * Point2f(cos(theta), sin(theta));

	return Point2f(0.5f, 0.5f) + 0.375f * uv;
}

std::shared_ptr<Filter> Filter::Create(const FilterParams& params) {
	if (params.type == FilterType::BoxFilter) {
		return std::make_shared<Box>();
	}
	else if (params.type == FilterType::TentFilter) {
		return std::make_shared<Tent>();
	}
	else if (params.type == FilterType::TriangleFilter) {
		return std::make_shared<Triangle>();
	}
	else if (params.type == FilterType::GaussianFilter) {
		return std::make_shared<Gaussian>();
	}

	return NULL;
}
