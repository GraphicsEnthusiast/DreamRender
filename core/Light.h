#pragma once

#include "Utils.h"
#include "Shape.h"
#include "Texture.h"

enum LightType {
	QuadAreaLight,
	SphereAreaLight,
	InfiniteAreaLight,
	TriangleMeshAreaLight
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

	inline virtual RGBSpectrum Radiance(const Vector3f& L) {
		return shape->GetMaterial()->Emit();
	}

	inline virtual float LightLuminance() {
		return Luminance(shape->GetMaterial()->Emit());
	}

	virtual RGBSpectrum EvaluateEnvironment(const Vector3f& L, float& pdf);

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info);

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

protected:
	LightType m_type;
	Shape* shape;
};

class QuadArea : public Light {
public:
	QuadArea(Shape* s, bool twoside = false) : Light(LightType::QuadAreaLight, s), twoSide(twoside) {}

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	bool twoSide;
};

class SphereArea : public Light {
public:
	SphereArea(Shape* s) : Light(LightType::SphereAreaLight, s) {}

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;
};

class InfiniteArea : public Light {
public:
	InfiniteArea(std::shared_ptr<Hdr> h, float sca = 1.0f);

	inline virtual RGBSpectrum Radiance(const Vector3f& L) override {
		Point2f planeUV = SphereToPlane(L);

		return hdr->GetColor(planeUV);
	}

	inline virtual float LightLuminance() override {
		return table.Sum();
	}

	virtual RGBSpectrum EvaluateEnvironment(const Vector3f& L, float& pdf) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Hdr> hdr;
	AliasTable2D table;
	float scale;
};

class TriangleMeshArea : public Light {
public:
	TriangleMeshArea(Shape* s);

	virtual RGBSpectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::vector<float> areas;
	AliasTable1D table;
};