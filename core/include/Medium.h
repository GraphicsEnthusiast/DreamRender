#pragma once

#include <Utils.h>
#include <Sampler.h>
#include <Intersection.h>

enum class PhaseFunctionType {
	Isotropic,        //各向同性的相函数
	HenyeyGreenstein, //亨尼-格林斯坦相函数
};

struct PhaseSample {
	PhaseSample(vec3 dir = vec3(0.0f), float pdf = -1.0f) :
		phase_dir(dir), phase_pdf(pdf) {}

	vec3 phase_dir;
	float phase_pdf;
};
typedef PhaseSample PhaseSampleError;

struct PhaseInfo {
	vec3 attenuation;
	float phase_pdf;
};

//相函数
class PhaseFunction {
public:

	virtual PhaseSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const = 0;
	virtual PhaseInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const = 0;

protected:
	PhaseFunction(PhaseFunctionType type) : type_(type) {}

private:
	PhaseFunctionType type_; //相函数类型
};

//各向同性的相函数
class IsotropicPhaseFunction : public PhaseFunction {
public:
	IsotropicPhaseFunction() : PhaseFunction(PhaseFunctionType::Isotropic) {}

	virtual PhaseSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const override;
	virtual PhaseInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const override;
};

//亨尼-格林斯坦相函数
class HenyeyGreensteinPhaseFunction : public PhaseFunction {
public:
	HenyeyGreensteinPhaseFunction(const vec3& g) : PhaseFunction(PhaseFunctionType::HenyeyGreenstein), g_(g) {}

	virtual PhaseSample Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const override;
	virtual PhaseInfo Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const override;

private:
	vec3 g_; //代表散射光线平均余弦的参数
};

enum class MediumType {
	Homogeneous, //均匀介质
};

class Medium {
public:
	virtual bool SampleDistance(float max_distance, float* distance, float* pdf, vec3* attenuation, Sampler* sampler) const = 0;
	virtual std::pair<vec3, float> EvalDistance(bool scattered, float distance) const = 0;
	virtual PhaseSample SamplePhaseFunction(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const = 0;
	virtual PhaseInfo EvalPhaseFunction(const vec3& V, const vec3& L, const IntersectionInfo& info) const = 0;

protected:
	Medium(MediumType type) : type_(type) {}

private:
	MediumType type_; //介质类型
};

//均匀介质
class HomogeneousMedium : public Medium {
public:
	HomogeneousMedium(const vec3& sigma_a, const vec3& sigma_s, PhaseFunction* phase_function);
	~HomogeneousMedium();

	bool SampleDistance(float max_distance, float* distance, float* pdf, vec3* attenuation, Sampler* sampler) const override;
	std::pair<vec3, float> EvalDistance(bool scattered, float distance) const override;
	PhaseSample SamplePhaseFunction(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const override;
	PhaseInfo EvalPhaseFunction(const vec3& V, const vec3& L, const IntersectionInfo& info) const override;

private:
	PhaseFunction* phase_function_;//相函数
	float medium_sampling_weight_; //抽样光线在介质内部发生散射的权重
	vec3 sigma_s_;                 //散射系数
	vec3 sigma_t_;                 //衰减系数
};