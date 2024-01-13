#pragma once

#include "Utils.h"
#include "Shape.h"
#include "Texture.h"
#include "Spectrum.h"

enum LightType {
	QuadAreaLight,
	SphereAreaLight,
	InfiniteAreaLight,
	TriangleMeshAreaLight
};

struct LightParams {
	LightType type;
	Shape* shape;
	bool twoside;
	std::shared_ptr<Hdr> hdr;
	float scale;
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

	inline virtual float LightLuminance() {
		return Luminance(shape->GetMaterial()->Emit());
	}

	virtual Spectrum EvaluateEnvironment(const Vector3f& L, float& pdf);

	virtual Spectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info);

	virtual Spectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

	static std::shared_ptr<Light> Create(const LightParams& params);

protected:
	LightType m_type;
	Shape* shape;
};

class QuadArea : public Light {
public:
	QuadArea(Shape* s, bool twoside = false) : Light(LightType::QuadAreaLight, s), twoSide(twoside) {}

	virtual Spectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual Spectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	bool twoSide;
};

class SphereArea : public Light {
public:
	SphereArea(Shape* s) : Light(LightType::SphereAreaLight, s) {}

	virtual Spectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual Spectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;
};

class InfiniteArea : public Light {
public:
	InfiniteArea(std::shared_ptr<Hdr> h, float sca = 1.0f);

	inline virtual float LightLuminance() override {
		return table.Sum();
	}

	virtual Spectrum EvaluateEnvironment(const Vector3f& L, float& pdf) override;

	virtual Spectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Hdr> hdr;
	AliasTable2D table;
	float scale;
};

class TriangleMeshArea : public Light {
public:
	TriangleMeshArea(Shape* s);

	virtual Spectrum Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual Spectrum Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::vector<float> areas;
	AliasTable1D table;
};