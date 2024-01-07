#include "Utils.h"
#include "Spectrum.h"
#include "Texture.h"
#include "Sampling.h"
#include "Sampler.h"
#include "Fresnel.h"
#include "Microfacet.h"
#include "Subsurface.h"

enum MaterialType {
	MediumBoundaryMaterial,
	DiffuseLightMaterial,
	DiffuseMaterial,
	ConductorMaterial,
	DielectricMaterial,
	PlasticMaterial,
	ThinDielectricMaterial,
	MetalWorkflowMaterial,
	ClearCoatedConductorMaterial,
	DiffuseTransmitterMaterial,
	MixtureMaterial,
	SubsurfaceMaterial
};

class Material {
public:
	Material(MaterialType type, std::shared_ptr<Texture> normal = NULL) : m_type(type), normalTexture(normal) {}

	inline MaterialType GetType() const {
		return m_type;
	}

	virtual RGBSpectrum Emit();

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) = 0;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) = 0;

protected:
	MaterialType m_type;
	std::shared_ptr<Texture> normalTexture;
};

class MediumBoundary : public Material {
public:
	MediumBoundary() : Material(MaterialType::MediumBoundaryMaterial) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;
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
	Diffuse(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness, std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::DiffuseMaterial, normal), albedoTexture(albedo), roughnessTexture(roughness) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture;
};

class Conductor : public Material {
public:
	Conductor(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, const RGBSpectrum& et, const RGBSpectrum& kk, 
		std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::ConductorMaterial, normal), albedoTexture(albedo), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v), eta(et), k(kk) {}

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
	Dielectric(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, float int_ior, float ext_ior, 
		std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::DielectricMaterial, normal), albedoTexture(albedo), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v), eta(int_ior / ext_ior) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	float eta;
};

class Plastic : public Material {
public:
	Plastic(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> specular, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, 
		float int_ior, float ext_ior, bool nonli, std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::PlasticMaterial, normal), albedoTexture(albedo), specularTexture(specular), roughnessTexture_u(roughness_u), 
		roughnessTexture_v(roughness_v), eta(int_ior / ext_ior), nonlinear(nonli) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> specularTexture;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	float eta;
	bool nonlinear;
};

class ThinDielectric : public Material {
public:
	ThinDielectric(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, float int_ior, float ext_ior,
		std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::ThinDielectricMaterial, normal), albedoTexture(albedo), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v), eta(int_ior / ext_ior) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	float eta;
};

class MetalWorkflow : public Material {
public:
	MetalWorkflow(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, std::shared_ptr<Texture> metallic,
		std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::MetalWorkflowMaterial, normal), albedoTexture(albedo), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v),
		metallicTexture(metallic) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	std::shared_ptr<Texture> metallicTexture;
};

class ClearCoatedConductor : public Material {
public:
	ClearCoatedConductor(std::shared_ptr<Conductor> con, std::shared_ptr<Texture> roughness_u, std::shared_ptr<Texture> roughness_v, float coatweight, 
		std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::ClearCoatedConductorMaterial, normal), conductor(con), roughnessTexture_u(roughness_u), roughnessTexture_v(roughness_v), coatWeight(coatweight) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Conductor> conductor;
	std::shared_ptr<Texture> roughnessTexture_u;
	std::shared_ptr<Texture> roughnessTexture_v;
	float coatWeight;
};

class DiffuseTransmitter : public Material {
public:
	DiffuseTransmitter(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> normal = NULL) :
		Material(MaterialType::DiffuseTransmitterMaterial, normal), albedoTexture(albedo) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Texture> albedoTexture;
};

class Mixture : public Material {
public:
	Mixture(std::shared_ptr<Material> m1, std::shared_ptr<Material> m2, float w) :
		Material(MaterialType::MixtureMaterial, NULL), material1(m1), material2(m2), weight(glm::clamp(w, 0.0f, 1.0f)) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) override;

private:
	std::shared_ptr<Material> material1;
	std::shared_ptr<Material> material2;
	float weight;
};