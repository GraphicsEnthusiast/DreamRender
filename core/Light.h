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

	virtual ~Light();

	inline LightType GetType() const {
		return m_type;
	}

	inline Shape* GetShape() const {
		return shape;
	}

	inline RGBSpectrum Radiance() {
		return shape->GetMaterial()->Emit();
	}

	virtual RGBSpectrum EvaluateEnvironment(const Vector3f& L, float& pdf);

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) = 0;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

protected:
	LightType m_type;
	Shape* shape;
};

class QuadArea : public Light {
public:
	QuadArea(Shape* s, bool twoside) : Light(LightType::QuadAreaLight, s), twoSide(twoside) {}

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	bool twoSide;
};

class SphereArea : public Light {
public:
	SphereArea(Shape* s) : Light(LightType::SphereAreaLight, s) {}

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;
};