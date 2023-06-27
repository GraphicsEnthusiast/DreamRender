#include <Medium.h>

PhaseSample IsotropicPhaseFunction::Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const {
	vec2 sample = sampler->Get2();
	float z = 1.0f - 2.0f * sample.x;
	float r = sqrt(std::max(0.0f, 1.0f - z * z));
	float phi = 2.0f * PI * sample.y;
	vec3 L{ r * cos(phi), r * sin(phi), z };

	float pdf = INV_4PI;

	return { L, pdf };
}

PhaseInfo IsotropicPhaseFunction::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const {
	vec3 attenuation(INV_4PI);
	float pdf = INV_4PI;

	return { attenuation, pdf };
}

PhaseSample HenyeyGreensteinPhaseFunction::Sample(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const {
	auto channel = std::min(static_cast<int>(sampler->Get1() * 3), 2);
	float g = g_[channel];

	float cos_theta = 0.0f;
	if (abs(g) < EPS) {
		cos_theta = 1.0f - 2.0f * sampler->Get1();
	}
	else {
		float sqr_term = (1.0f - g * g) / (1.0f - g + 2.0f * g * sampler->Get1());
		cos_theta = (1.0f + g * g - sqr_term * sqr_term) / (2.0f * g);
	}

	float pdf = 0.0f;
	vec3 attenuation;
	for (int dim = 0; dim < 3; ++dim) {
		float temp = 1.0f + g_[dim] * g_[dim] + 2.0f * g_[dim] * cos_theta;
		attenuation[dim] = (INV_4PI) * (1.0f - g_[dim] * g_[dim]) / (temp * sqrt(temp));
		pdf += attenuation[dim];
	}
	pdf *= (1.0f / 3.0f);
	if (pdf <= 0.0f) {
		return PhaseSampleError();
	}

	float sin_theta = sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
	float phi = 2.0f * PI * sampler->Get1();
	vec3 L(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
	L = ToWorld(L, V);

	return { L, pdf };
}

PhaseInfo HenyeyGreensteinPhaseFunction::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) const {
	float cos_theta = dot(L, V);
	float pdf = 0.0f;
	vec3 attenuation;
	for (int dim = 0; dim < 3; ++dim) {
		float temp = 1.0f + g_[dim] * g_[dim] + 2.0f * g_[dim] * cos_theta;
		attenuation[dim] = (INV_4PI) * (1.0f - g_[dim] * g_[dim]) / (temp * sqrt(temp));
		pdf += attenuation[dim];
	}
	pdf *= (1.0f / 3.0f);
	if (pdf <= 0.0f) {
		return { vec3(0.0f), -1.0f };
	}

	return { attenuation, pdf };
}

HomogeneousMedium::HomogeneousMedium(const vec3& albedo, const vec3& sigma_t, PhaseFunction* phase_function)
	:
	sigma_t_(sigma_t),
	albedo_(albedo),
	medium_sampling_weight_(0.0f),
	phase_function_(phase_function) {
	for (int dim = 0; dim < 3; ++dim) {
		if (albedo[dim] > medium_sampling_weight_ && sigma_t_[dim] != 0.0f) {
			medium_sampling_weight_ = albedo[dim];
		}
	}
	if (medium_sampling_weight_ > 0.0f) {
		medium_sampling_weight_ = std::max(medium_sampling_weight_, 0.5f);
	}
}

HomogeneousMedium::~HomogeneousMedium() {
	if (phase_function_) {
		delete phase_function_;
		phase_function_ = nullptr;
	}
}

bool HomogeneousMedium::SampleDistance(float max_distance, float* distance, float* trans_pdf, vec3* transmittance, Sampler* sampler) const {
	bool scattered = false;
	float xi_1 = sampler->Get1();
	if (xi_1 < medium_sampling_weight_) { //抽样光线在介质内部是否发生散射
		xi_1 /= medium_sampling_weight_;
		const int channel = std::min(static_cast<int>(sampler->Get1() * 3), 2);
		*distance = -std::log(1.0f - xi_1) / sigma_t_[channel];
		if (*distance < max_distance) { //光线在介质内部发生了散射
			*trans_pdf = 0.0f;
			for (int dim = 0; dim < 3; ++dim) {
				*trans_pdf += sigma_t_[dim] * std::exp(-sigma_t_[dim] * *distance);
			}
			*trans_pdf *= medium_sampling_weight_ * (1.0f / 3.0f);
			scattered = true;
		}
	}
	if (!scattered) { //光线在介质内部没有发生散射
		*distance = max_distance;
		*trans_pdf = 0.0f;
		for (int dim = 0; dim < 3; ++dim) {
			*trans_pdf += std::exp(-sigma_t_[dim] * *distance);
		}
		*trans_pdf = medium_sampling_weight_ * (1.0f / 3.0f) * *trans_pdf + (1.0f - medium_sampling_weight_);
	}

	bool valid = false;
	for (int dim = 0; dim < 3; ++dim) {
		(*transmittance)[dim] = std::exp(-sigma_t_[dim] * *distance);
		if ((*transmittance)[dim] > 0.0f) {
			valid = true;
		}
	}
	if (scattered) {
		vec3 sigma_s_ = albedo_ * sigma_t_;
		*transmittance *= sigma_s_;
	}
	if (!valid) {
		*transmittance = vec3(0.0f);
	}

	return scattered;
}

std::pair<vec3, float> HomogeneousMedium::EvalDistance(bool scattered, float distance) const {
	vec3 attenuation(0.0f);
	float pdf = 0.0f;
	bool valid = false;
	for (int dim = 0; dim < 3; ++dim) {
		attenuation[dim] = std::exp(-sigma_t_[dim] * distance);
		if (attenuation[dim] > 0.0f) {
			valid = true;
		}
	}

	vec3 sigma_s_ = albedo_ * sigma_t_;
	if (scattered) {
		for (int dim = 0; dim < 3; ++dim) {
			pdf += sigma_t_[dim] * attenuation[dim];
		}
		pdf *= medium_sampling_weight_ * (1.0f / 3.0f);
		attenuation *= sigma_s_;
	}
	else {
		for (int dim = 0; dim < 3; ++dim) {
			pdf += attenuation[dim];
		}
		pdf = medium_sampling_weight_ * (1.0f / 3.0f) * pdf + (1.0f - medium_sampling_weight_);
	}

	if (!valid) {
		attenuation = vec3(0.0f);
	}

	return { attenuation, pdf };
}

PhaseSample HomogeneousMedium::SamplePhaseFunction(const vec3& V, const IntersectionInfo& info, Sampler* sampler) const {
	return phase_function_->Sample(V, info, sampler);
}

PhaseInfo HomogeneousMedium::EvalPhaseFunction(const vec3& V, const vec3& L, const IntersectionInfo& info) const {
	return phase_function_->Eval(V, L, info);
}