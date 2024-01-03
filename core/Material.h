#include "Utils.h"
#include "Spectrum.h"
#include "Texture.h"
#include "Sampling.h"
#include "Sampler.h"

namespace Fresnel {
	float FresnelSchlick(float f0, float VdotH);

	Vector3f FresnelSchlick(Vector3f f0, float VdotH);

	Vector3f FresnelConductor(const Vector3f& V, const Vector3f& H, const Vector3f& eta_r, const Vector3f& eta_i);

	Vector3f AverageFresnelConductor(Vector3f eta, Vector3f k);

	float FresnelDielectric(const Vector3f& V, const Vector3f& H, float eta_inv);

	float AverageFresnelDielectric(float eta);
}

namespace GGX {
	float GeometrySmith1(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	float DistributionGGX(const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	float DistributionVisibleGGX(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	Vector3f SampleVisibleGGX(const Vector3f& N, const Vector3f& V, float alpha_u, float alpha_v, const Point2f& sample);
}

enum MaterialType {
	DiffuseLightMaterial,
	DiffuseMaterial,
	ConductorMaterial,
	DielectricMaterial,
	PlasticMaterial,
	MetalWorkflowMaterial,
	ThinDielectricMaterial,
	ClearCoatedConductorMaterial
};

class Material {
public:
	Material(MaterialType type) : m_type(type) {}

	inline MaterialType GetType() const {
		return m_type;
	}

	virtual RGBSpectrum Emit();

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) = 0;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

protected:
	MaterialType m_type;
};

class DiffuseLight : public Material {
public:
	DiffuseLight(const RGBSpectrum& rad) : Material(MaterialType::DiffuseLightMaterial), radiance(rad) {}

	virtual RGBSpectrum Emit() override;

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	const RGBSpectrum radiance;
};

class Diffuse : public Material {
public:
	Diffuse(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness) : Material(MaterialType::DiffuseMaterial), albedoTexture(albedo), roughnessTexture(roughness) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture;
};