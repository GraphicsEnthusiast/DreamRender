#pragma once

#include "Utils.h"

class AliasTable1D {
public:
	AliasTable1D() = default;

	AliasTable1D(const std::vector<float>& distrib);

	int Sample(const Point2f& sample);

	inline float Sum() const { 
		return sumDistrib; 
	}

	inline std::vector<std::pair<int, float>> GetTable() const { 
		return table; 
	}

private:
	typedef std::pair<int, float> Element;

private:
	std::vector<Element> table;
	float sumDistrib;
};

class AliasTable2D {
public:
	AliasTable2D() = default;

	AliasTable2D(const std::vector<float>& distrib, int width, int height);

	std::pair<int, int> Sample(const Point2f& sample1, const Point2f& sample2);

	inline float Sum() const { 
		return colTable.Sum(); 
	}

private:
	std::vector<AliasTable1D> rowTables;
	AliasTable1D colTable;
};

inline Point2f UniformSampleDisk(const Point2f& sample, float radius) {
	float sampleY = sample.x;
	float sampleX = sample.y;

	float theta = 2.0f * PI * sampleX;
	float r = sampleY * radius;

	return Vector2f(r * std::cos(theta), r * std::sin(theta));
}

inline Vector3f PlaneToSphere(const Point2f& uv) {
	Point2f xy = uv;

	float phi = 2.0f * PI * (xy.x - 0.5f);// [-pi ~ pi]
	float theta = PI * (xy.y - 0.5f);// [-pi/2 ~ pi/2]   

	Vector3f L(cos(theta) * cos(phi), sin(theta), cos(theta) * sin(phi));

	return normalize(L);
}

inline Point2f SphereToPlane(const Vector3f& v) {
	Point2f uv(std::atan2(v.z, v.x), std::asin(v.y));
	uv *= Point2f(INV_2PI, INV_PI);
	uv += 0.5f;

	return uv;
}

inline Vector3f UniformSampleCone(const Point2f& sample, float cos_angle) {
	Vector3f p(0.0f);

	float phi = 2.0f * PI * sample.x;
	float cos_theta = glm::mix(cos_angle, 1.0f, sample.y);
	float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

	p.x = sin_theta * std::cos(phi);
	p.y = sin_theta * std::sin(phi);
	p.z = cos_theta;

	return normalize(p);
}

inline float UniformPdfCone(float cos_angle) {
	return INV_2PI /  (1.0f - cos_angle);
}

inline Vector3f CosineSampleHemisphere(const Point2f& sample) {
	Vector3f p(0.0f);

	// Uniformly sample disk
	float r = std::sqrt(sample.x);
	float phi = 2.0f * PI * sample.y;
	p.x = r * std::cos(phi);
	p.y = r * std::sin(phi);

	// Project up to hemisphere
	p.z = std::sqrt(std::max(0.0f, 1.0f - p.x * p.x - p.y * p.y));

	return normalize(p);
}

inline float CosinePdfHemisphere(float NdotL) {
	return NdotL * INV_PI;
}

inline Vector3f UniformSampleSphere(const Point2f& sample) {
	Vector3f p(0.0f);

	float phi = 2.0f * PI * sample.x;
	float cos_theta = 1.0f - 2.0f * sample.y;
	float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

	p.x = sin_theta * cos(phi);
	p.y = sin_theta * sin(phi);
	p.z = cos_theta;

	return normalize(p);
}

inline float UniformPdfSphere() {
	return INV_4PI;
}

inline Vector3f UniformSampleHemisphere(const Point2f& sample) {
	float r = std::sqrt(std::max(0.0f, 1.0f - sample.x * sample.x));
	float phi = 2.0f * PI * sample.y;
	Vector3f p(r * cos(phi), r * sin(phi), sample.x);

	return normalize(p);
}

inline float UniformPdfHemisphere() {
	return INV_2PI;
}

template <typename Predicate>
inline int FindInterval(int size, const Predicate& pred) {
	int first = 0, len = size;
	while (len > 0) {
		int half = len >> 1, middle = first + half;
		// Bisect range based on value of _pred_ at _middle_
		if (pred(middle)) {
			first = middle + 1;
			len -= half + 1;
		}
		else {
			len = half;
		}
	}

	return glm::clamp(first - 1, 0, size - 2);
}