#include "Utils.h"
#include "Spectrum.h"
#include "Texture.h"
#include "Sampling.h"
#include "Sampler.h"

enum MaterialType {
	DiffuseMaterial,
	ConductorMaterial,
	DielectricMaterial,
	PlasticMaterial,
	MetalWorkflowMaterial,
	ThinDielectricMaterial,
	ClearCoatedConductorMaterial,
	DiffuseTransmitterMaterial
};

class Material {
public:
	Material(MaterialType type) : m_type(type) {}

	virtual inline MaterialType GetType() const {
		return m_type;
	}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) = 0;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) = 0;

protected:
	MaterialType m_type;
};

class Diffuse : public Material {
public:
	Diffuse(std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> roughness) :
		Material(MaterialType::DiffuseMaterial), albedoTexture(albedo), roughnessTexture(roughness) {}

	virtual RGBSpectrum Evaluate(const Vector3f& V, const Vector3f& L, float& pdf, const IntersectionInfo& info) override;

	virtual RGBSpectrum Sample(const Vector3f& V, Vector3f& L, float& pdf, const IntersectionInfo& info, Sampler* sampler) override;

protected:
	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> roughnessTexture;
};