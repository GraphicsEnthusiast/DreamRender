#include "Utils.h"
#include "Spectrum.h"
#include "Texture.h"
#include "Sampling.h"
#include "Sampler.h"

namespace Fresnel {
	float FresnelSchlick(float f0, float VdotH);

	RGBSpectrum FresnelSchlick(const RGBSpectrum& f0, float VdotH);

	RGBSpectrum FresnelConductor(const Vector3f& V, const Vector3f& H, const RGBSpectrum& eta_r, const RGBSpectrum& eta_i);

	RGBSpectrum AverageFresnelConductor(const RGBSpectrum& eta, const RGBSpectrum& k);

	float FresnelDielectric(const Vector3f& V, const Vector3f& H, float eta_inv);

	float AverageFresnelDielectric(float eta);
}

namespace GGX {
	float GeometrySmith1(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	float Distribution(const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	float DistributionVisible(const Vector3f& V, const Vector3f& H, const Vector3f& N, float alpha_u, float alpha_v);

	Vector3f SampleVisible(const Vector3f& N, const Vector3f& V, float alpha_u, float alpha_v, const Point2f& sample);
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
	RGBSpectrum radiance;
};

class Diffuse : public Material {
public:
	Diffuse(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness) : 
		Material(MaterialType::DiffuseMaterial), albedoTexture(albedo), roughnessTexture(roughness) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture;
};

class Conductor : public Material {
public:
	Conductor(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, const RGBSpectrum& et, const RGBSpectrum& kk) :
		Material(MaterialType::ConductorMaterial), albedoTexture(albedo), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v), eta(et), k(kk) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	RGBSpectrum eta;
	RGBSpectrum k;
};

class Dielectric : public Material {
public:
	Dielectric(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, float int_ior, float ext_ior) :
		Material(MaterialType::DielectricMaterial), albedoTexture(albedo), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v), eta(int_ior / ext_ior) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	float eta;
};