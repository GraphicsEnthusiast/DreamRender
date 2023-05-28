#pragma once

#include <Utils.h>
#include <Texture.h>
#include <Sampler.h>

//材质类型
enum class MaterialType {
	DiffuseLight,          //漫反射光源的材质
	SmoothDiffuse,         //平滑的理想漫反射材质，由朗伯模型描述
	RoughDiffuse,          //粗糙的理想漫反射材质，由 Oren–Nayar Reflectance Model 描述
	SmoothDielectric,      //平滑的电介质
	RoughDielectric,       //粗糙的电介质
	ThinDielectric,        //薄的电介质
	SmoothConductor,       //平滑的导体
	RoughConductor,        //粗糙的导体
	ClearcoatedConductor,  //表面有一层电介质的粗糙导体
	SmoothPlastic,         //平滑的塑料
	RoughPlastic,          //粗糙的塑料
	MetalWorkflow,         //金属工作流，由 Cook - Torrance 描述  
	DisneyDiffuse,
	DisneyMetal,
	DisneyClearcoat,
	DisneyGlass,
	DisneySheen,
	DisneyPrinciple,       //Disney 原则 BSDF
};

struct BsdfSample {
	BsdfSample(vec3 dir = vec3(0.0f), float pdf = -1.0f) : bsdf_dir(dir), bsdf_pdf(pdf) {}

	vec3 bsdf_dir;
	float bsdf_pdf;
};
typedef BsdfSample BsdfSampleError;

struct EvalInfo {
	vec3 bsdf;
	float costheta;
	float bsdf_pdf;
};

inline vec3 NormalFormTangentToWorld(vec3 n, vec3 tangentNormal) {
	tangentNormal = normalize(tangentNormal * 2.0f - 1.0f);

	//Orthonormal Basis
	vec3 UpVector = abs(n.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
	vec3 TangentX = normalize(cross(UpVector, n));
	vec3 TangentY = normalize(cross(n, TangentX));

	return normalize(TangentX * tangentNormal.x + TangentY * tangentNormal.y + n * tangentNormal.z);
}

namespace Fresnel {
	float SchlickWeight(float u);
	float FresnelSchlick(float f0, float VdotH);
	vec3 FresnelSchlick(vec3 f0, float VdotH);

	vec3 FresnelConductor(const vec3& V, const vec3& H, const vec3& eta_r, const vec3& eta_i);
	vec3 AverageFresnelConductor(vec3 eta, vec3 k);

	float FresnelDielectric(const vec3& V, const vec3& H, float eta_inv);
	float AverageFresnelDielectric(float eta);
}

namespace CosWeight {
	vec3 Sample(const vec3& N, const vec2& sample);
}

namespace GGX {
	float GeometrySmith_1(const vec3& V, const vec3& H, const vec3& N, float alpha_u, float alpha_v);
	float DistributionGGX(const vec3& H, const vec3& N, float alpha_u, float alpha_v);
	float DistributionVisibleGGX(const vec3& V, const vec3& H, const vec3& N, float alpha_u, float alpha_v);
	vec3 Sample(const vec3& N, float alpha_u, float alpha_v, const vec2& sample);
	vec3 SampleVisible(const vec3& N, const vec3& V, float alpha_u, float alpha_v, const vec2& sample);
}

namespace GTR1 {
	float DistributionGTR1(float NdotH, float a);
	vec3 Sample(vec3 V, vec3 N, float alpha, vec2 sample);
}

class KullaConty {
public:
	KullaConty(float (*G1)(const vec3& V, const vec3& H, const vec3& N, float alpha_u, float alpha_v) = GGX::GeometrySmith_1,
		vec3(*Sample)(const vec3& N, float alpha_u, float alpha_v, const vec2& sample) = GGX::Sample);

	//已知样本容量，生成前两维的哈默斯利序列
	inline vec2 Hammersley(uint32_t i, uint32_t N) {
		uint32_t bits = (i << 16u) | (i >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		auto rdi = static_cast<float>(float(bits) * 2.3283064365386963e-10f);

		return { float(i) / float(N), rdi };
	}

	float Albedo(float cos_theta, float roughness) const;
	float AverageAlbedo(float roughness) const;
	vec3 EvalMultipleScatter(float NdotL, float NdotV, float roughness, vec3 F_avg);

private:
	static constexpr int kLutResolution = 256;//预计算纹理贴图的精度
	static constexpr float kStep = 1.0f / kLutResolution;
	static constexpr int kSampleCount = 512;
	static constexpr float kSampleCountInv = 1.0f / kSampleCount;
	array<array<float, kLutResolution>, kLutResolution> albedo_lut;//反照率查找表
	array<float, kLutResolution> albedo_avg_lut;//平均反照率查找表
};

class Material {
public:
	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) = 0;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) = 0;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) = 0;
	virtual vec3 Emitted(const IntersectionInfo& info) { return vec3(0.0f); }
	virtual bool IsDelta() const noexcept { return false; }

public:
	MaterialType m_type;
};

class DiffuseLight : public Material {
public:
	DiffuseLight(shared_ptr<Texture> e) : emittedTexture(e) { m_type = MaterialType::DiffuseLight; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 Emitted(const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> emittedTexture;
};

class SmoothDiffuse : public Material {
public:
	SmoothDiffuse(shared_ptr<Texture> a) : albedoTexture(a) { m_type = MaterialType::SmoothDiffuse; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
};

class SmoothConductor : public Material {
public:
	SmoothConductor(shared_ptr<Texture> a, vec3 e, vec3 _k, shared_ptr<Texture> n = NULL) :
		albedoTexture(a), eta(e), k(_k), normalTexture(n) { m_type = MaterialType::SmoothConductor; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual bool IsDelta() const noexcept override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> normalTexture;
	vec3 eta;//材质相对折射率的实部
	vec3 k;//材质相对折射率的虚部（消光系数）
};

class SmoothDielectric : public Material {
public:
	SmoothDielectric(shared_ptr<Texture> a, float int_ior, float ext_ior, shared_ptr<Texture> n = NULL) :
		albedoTexture(a), eta(int_ior / ext_ior), normalTexture(n) { m_type = MaterialType::SmoothDielectric; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual bool IsDelta() const noexcept override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> normalTexture;
	float eta;//介质折射率与外部折射率之比
};

class SmoothPlastic : public Material {
public:
	SmoothPlastic(shared_ptr<Texture> d, shared_ptr<Texture> s, float int_ior, float ext_ior, bool non, shared_ptr<Texture> n = NULL) :
		diffuseTexture(d), specularTexture(s), eta_inv(ext_ior / int_ior), nonlinear(non), 
		fdr(Fresnel::AverageFresnelDielectric(int_ior / ext_ior)), normalTexture(n) { m_type = MaterialType::SmoothPlastic; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	bool nonlinear;//是否考虑因内部散射而引起的非线性色移
	float fdr;//漫反射菲涅尔项的平均值
	float eta_inv;//外部折射率与介质折射率之比
	shared_ptr<Texture> diffuseTexture;//漫反射系数
	shared_ptr<Texture> specularTexture;//镜面反射系数
	shared_ptr<Texture> normalTexture;
};

class RoughDiffuse : public Material {
public:
	RoughDiffuse(shared_ptr<Texture> a, shared_ptr<Texture> r) : albedoTexture(a), roughnessTexture(r) { m_type = MaterialType::RoughDiffuse; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
};

class RoughConductor : public Material {
public:
	RoughConductor(shared_ptr<KullaConty> ku, shared_ptr<Texture> a, shared_ptr<Texture> r, shared_ptr<Texture> an, vec3 e, vec3 _k) :
		albedoTexture(a), roughnessTexture(r), anisotropyTexture(an), eta(e), k(_k) ,kulla_conty(ku) {
		F_avg = Fresnel::AverageFresnelConductor(eta, k);
		m_type = MaterialType::RoughConductor;
	}

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> anisotropyTexture;
	vec3 eta;//材质相对折射率的实部
	vec3 k;//材质相对折射率的虚部（消光系数）
	shared_ptr<KullaConty> kulla_conty;
	vec3 F_avg;
};

class RoughDielectric : public Material {
public:
	RoughDielectric(shared_ptr<KullaConty> ku, shared_ptr<Texture> a, shared_ptr<Texture> r, shared_ptr<Texture> an, float int_ior, float ext_ior) :
		albedoTexture(a), roughnessTexture(r), anisotropyTexture(an), eta(int_ior / ext_ior), eta_inv(ext_ior / int_ior), kulla_conty(ku) {
		F_avg = Fresnel::AverageFresnelDielectric(eta);
		F_avg_inv = Fresnel::AverageFresnelDielectric(eta_inv);
		ratio_t = (1.0f - F_avg) * (1.0f - F_avg_inv) * sqr(eta) / ((1.0f - F_avg) + (1.0f - F_avg_inv) * sqr(eta));
		ratio_t_inv = (1.0f - F_avg_inv) * (1.0f - F_avg) * sqr(eta_inv) / ((1.0f - F_avg_inv) + (1.0f - F_avg) * sqr(eta_inv));
		m_type = MaterialType::RoughDielectric;
	}

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> anisotropyTexture;
	float eta;//介质折射率与外部折射率之比
	float eta_inv;//外部折射率与介质折射率之比
	shared_ptr<KullaConty> kulla_conty;
	float F_avg;
	float F_avg_inv;
	float ratio_t;
	float ratio_t_inv;
};

class RoughPlastic : public Material {
public:
	RoughPlastic(shared_ptr<KullaConty> ku, shared_ptr<Texture> d, shared_ptr<Texture> s, shared_ptr<Texture> r, shared_ptr<Texture> an, float int_ior, float ext_ior, bool non) :
		diffuseTexture(d), specularTexture(s), roughnessTexture(r), anisotropyTexture(an), eta_inv(ext_ior / int_ior), nonlinear(non), kulla_conty(ku),
		fdr(Fresnel::AverageFresnelDielectric(int_ior / ext_ior)) {
		m_type = MaterialType::RoughPlastic;
	}

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	bool nonlinear;//是否考虑因内部散射而引起的非线性色移
	float fdr;//漫反射菲涅尔项的平均值
	float eta_inv;//外部折射率与介质折射率之比
	shared_ptr<Texture> diffuseTexture;//漫反射系数
	shared_ptr<Texture> specularTexture;//镜面反射系数
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> anisotropyTexture;
	shared_ptr<KullaConty> kulla_conty;
};

class ClearcoatedConductor : public Material {
public:
	ClearcoatedConductor(shared_ptr<RoughConductor> con, shared_ptr<Texture> r_u, shared_ptr<Texture> r_v, float cle) : 
		conductor(con), roughnessTexture_u(r_u), roughnessTexture_v(r_v), clear_coat(cle) { m_type = MaterialType::ClearcoatedConductor; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<RoughConductor> conductor;
	shared_ptr<Texture> roughnessTexture_u;
	shared_ptr<Texture> roughnessTexture_v;
	float clear_coat;
};

class ThinDielectric : public Material {
public:
	ThinDielectric(shared_ptr<Texture> a, float int_ior, float ext_ior, shared_ptr<Texture> n = NULL) :
		albedoTexture(a), eta_inv(ext_ior / int_ior), normalTexture(n) { m_type = MaterialType::ThinDielectric; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual bool IsDelta() const noexcept override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> normalTexture;
	float eta_inv;//外部折射率与介质折射率之比
};

class MetalWorkflow : public Material {
public:
	MetalWorkflow(shared_ptr<Texture> a, shared_ptr<Texture> r, shared_ptr<Texture> m, shared_ptr<Texture> n) :
		albedoTexture(a), roughnessTexture(r), metallicTexture(m), metallicRoughnessTexture(NULL), normalTexture(n) { m_type = MaterialType::MetalWorkflow; }
	MetalWorkflow(shared_ptr<Texture> a, shared_ptr<Texture> metallicRoughness, shared_ptr<Texture> n) :
		albedoTexture(a), roughnessTexture(NULL), metallicTexture(NULL), metallicRoughnessTexture(metallicRoughness), normalTexture(n) {
		m_type = MaterialType::MetalWorkflow;
	}

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> metallicTexture;
	shared_ptr<Texture> normalTexture;
	shared_ptr<Texture> metallicRoughnessTexture;
};

class DisneyDiffuse : public Material {
public:
	DisneyDiffuse(shared_ptr<Texture> a, shared_ptr<Texture> r, shared_ptr<Texture> s) :
		albedoTexture(a), roughnessTexture(r), subsurfaceTexture(s) { m_type = MaterialType::DisneyDiffuse; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> subsurfaceTexture;
};

class DisneyMetal : public Material {
public:
	DisneyMetal(shared_ptr<Texture> a, shared_ptr<Texture> r, shared_ptr<Texture> an, shared_ptr<Texture> metallic,
		shared_ptr<Texture> specular, shared_ptr<Texture> specularTint) :
		albedoTexture(a), roughnessTexture(r), anisotropicTexture(an), metallicTexture(metallic),
	specularTexture(specular), specularTintTexture(specularTint) { m_type = MaterialType::DisneyMetal; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> anisotropicTexture;
	shared_ptr<Texture> metallicTexture;
	shared_ptr<Texture> specularTexture;
	shared_ptr<Texture> specularTintTexture;
};

class DisneyClearcoat : public Material {
public:
	DisneyClearcoat(shared_ptr<Texture> c) : clearcoatGlossTexture(c) { m_type = MaterialType::DisneyClearcoat; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> clearcoatGlossTexture;
};

class DisneyGlass : public Material {
public:
	DisneyGlass(shared_ptr<Texture> a, shared_ptr<Texture> r, shared_ptr<Texture> an, float int_ior, float ext_ior) :
		albedoTexture(a), roughnessTexture(r), anisotropicTexture(an), eta(int_ior / ext_ior) { m_type = MaterialType::DisneyGlass; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> roughnessTexture;
	shared_ptr<Texture> anisotropicTexture;
	float eta;
};

class DisneySheen : public Material {
public:
	DisneySheen(shared_ptr<Texture> a, shared_ptr<Texture> s) : albedoTexture(a), sheenTintTexture(s) { m_type = MaterialType::DisneySheen; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<Texture> albedoTexture;
	shared_ptr<Texture> sheenTintTexture;
};

class DisneyPrinciple : public Material {
public:
	DisneyPrinciple(shared_ptr<DisneyDiffuse> diff, shared_ptr<DisneyMetal> met, shared_ptr<DisneyClearcoat> clear,
		shared_ptr<DisneyGlass> gla, shared_ptr<DisneySheen> she, shared_ptr<Texture> metallic, shared_ptr<Texture> specularTransmission,
		shared_ptr<Texture> sheen, shared_ptr<Texture> clearcoat) : 
		diffuse_m(diff), metal_m(met), clearcoat_m(clear), glass_m(gla), sheen_m(she), 
		metallicTexture(metallic), specularTransmissionTexture(specularTransmission), sheenTexture(sheen),
	    clearcoatTexture(clearcoat) { m_type = MaterialType::DisneyPrinciple; }

	virtual BsdfSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) override;
	virtual EvalInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) override;
	virtual vec3 GetAlbedo(const IntersectionInfo& info) override;

private:
	shared_ptr<DisneyDiffuse> diffuse_m;
	shared_ptr<DisneyMetal> metal_m;
	shared_ptr<DisneyClearcoat> clearcoat_m;
	shared_ptr<DisneyGlass> glass_m;
	shared_ptr<DisneySheen> sheen_m;
	shared_ptr<Texture> metallicTexture;
	shared_ptr<Texture> specularTransmissionTexture;
	shared_ptr<Texture> sheenTexture;
	shared_ptr<Texture> clearcoatTexture;
};