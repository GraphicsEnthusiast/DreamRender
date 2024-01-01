#pragma once

#include "Utils.h"
#include "Shape.h"

enum LightType {
	QuadAreaLight,
	SphereAreaLight,
	InfiniteAreaLight
};

class Light {
public:
	Light(LightType type, Shape* s) : m_type(type), shape(s) {}

	virtual ~Light() {
		if (shape != NULL) {
			delete shape;
			shape = NULL;
		}
	}

	inline LightType GetType() const {
		return m_type;
	}

	virtual RGBSpectrum EvaluateEnvironment(const Vector3f& L, float& pdf);

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info, float distance) = 0;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) = 0;

protected:
	LightType m_type;
	Shape* shape;
};

class QuadArea : Light {
public:
	QuadArea(Shape* s, bool doubleside) : Light(LightType::QuadAreaLight, s), doubleSide(doubleside) {}

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info, float distance) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) override;

private:
	bool doubleSide;
};