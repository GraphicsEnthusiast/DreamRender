#include <Material.h>

float Fresnel::SchlickWeight(float u) {
	float m = glm::clamp(1.0f - u, 0.0f, 1.0f);
	float m2 = m * m;

	return m2 * m2 * m;
}

float Fresnel::FresnelSchlick(float f0, float VdotH) {
	float tmp = 1.0f - glm::clamp(VdotH, 0.0f, 1.0f);
	float tmp2 = tmp * tmp;
	float Fc = tmp2 * tmp2 * tmp;

	return f0 + (1.0f - f0) * Fc;
}

vec3 Fresnel::FresnelSchlick(vec3 f0, float VdotH) {
	float tmp = 1.0f - glm::clamp(VdotH, 0.0f, 1.0f);
	float tmp2 = tmp * tmp;
	float Fc = tmp2 * tmp2 * tmp;

	return f0 + (1.0f - f0) * Fc;
}

vec3 Fresnel::FresnelConductor(const vec3& V, const vec3& H, const vec3& eta_r, const vec3& eta_i) {
	vec3 N = H;
	float cos_v_n = dot(V, N),
		cos_v_n_2 = cos_v_n * cos_v_n,
		sin_v_n_2 = 1.0f - cos_v_n_2,
		sin_v_n_4 = sin_v_n_2 * sin_v_n_2;

	vec3 temp_1 = eta_r * eta_r - eta_i * eta_i - sin_v_n_2,
		a_2_pb_2 = temp_1 * temp_1 + 4.0f * eta_i * eta_i * eta_r * eta_r;
	for (int i = 0; i < 3; i++) {
		a_2_pb_2[i] = sqrt(std::max(0.0f, a_2_pb_2[i]));
	}
	vec3 a = 0.5f * (a_2_pb_2 + temp_1);
	for (int i = 0; i < 3; i++) {
		a[i] = sqrt(std::max(0.0f, a[i]));
	}
	vec3 term_1 = a_2_pb_2 + sin_v_n_2,
		term_2 = 2.0f * cos_v_n * a,
		term_3 = a_2_pb_2 * cos_v_n_2 + sin_v_n_4,
		term_4 = term_2 * sin_v_n_2,
		r_s = (term_1 - term_2) / (term_1 + term_2),
		r_p = r_s * (term_3 - term_4) / (term_3 + term_4);

	return 0.5f * (r_s + r_p);
}

vec3 Fresnel::AverageFresnelConductor(vec3 eta, vec3 k) {
	auto reflectivity = vec3(0.0f),
		edgetint = vec3(0.0f);
	float temp1 = 0.0f, temp2 = 0.0f, temp3 = 0.0f;
	for (int i = 0; i < 3; i++) {
		reflectivity[i] = (sqr(eta[i] - 1.0f) + sqr(k[i])) / (sqr(eta[i] + 1.0f) + sqr(k[i]));
		temp1 = 1.0f + sqrt(reflectivity[i]);
		temp2 = 1.0f - sqrt(reflectivity[i]);
		temp3 = (1.0f - reflectivity[i]) / (1.0f + reflectivity[i]);
		edgetint[i] = (temp1 - eta[i] * temp2) / (temp1 - temp3 * temp2);
	}

	return vec3(0.087237f) +
		0.0230685f * edgetint -
		0.0864902f * edgetint * edgetint +
		0.0774594f * edgetint * edgetint * edgetint +
		0.782654f * reflectivity -
		0.136432f * reflectivity * reflectivity +
		0.278708f * reflectivity * reflectivity * reflectivity +
		0.19744f * edgetint * reflectivity +
		0.0360605f * edgetint * edgetint * reflectivity -
		0.2586f * edgetint * reflectivity * reflectivity;
}

float Fresnel::FresnelDielectric(const vec3& V, const vec3& H, float eta_inv) {
	float cos_theta_i = abs(dot(V, H));
	float cos_theta_t_2 = 1.0f - sqr(eta_inv) * (1.0f - sqr(cos_theta_i));
	if (cos_theta_t_2 <= 0.0f) {
		return 1.0f;
	}
	else {
		float cos_theta_t = sqrt(cos_theta_t_2),
			Rs_sqrt = (eta_inv * cos_theta_i - cos_theta_t) / (eta_inv * cos_theta_i + cos_theta_t),
			Rp_sqrt = (cos_theta_i - eta_inv * cos_theta_t) / (cos_theta_i + eta_inv * cos_theta_t);

		return (Rs_sqrt * Rs_sqrt + Rp_sqrt * Rp_sqrt) / 2.0f;
	}
}

float Fresnel::AverageFresnelDielectric(float eta) {
	if (eta < 1.0f) {
		/* Fit by Egan and Hilgeman (1973). Works reasonably well for
			"normal" IOR values (<2).
			Max rel. error in 1.0 - 1.5 : 0.1%
			Max rel. error in 1.5 - 2   : 0.6%
			Max rel. error in 2.0 - 5   : 9.5%
		*/
		return -1.4399f * (eta * eta) + 0.7099f * eta + 0.6681f + 0.0636f / eta;
	}
	else {
		/* Fit by d'Eon and Irving (2011)

			Maintains a good accuracy even for unistic IOR values.

			Max rel. error in 1.0 - 2.0   : 0.1%
			Max rel. error in 2.0 - 10.0  : 0.2%
		*/
		float inv_eta = 1.0f / eta,
			inv_eta_2 = inv_eta * inv_eta,
			inv_eta_3 = inv_eta_2 * inv_eta,
			inv_eta_4 = inv_eta_3 * inv_eta,
			inv_eta_5 = inv_eta_4 * inv_eta;

		return 0.919317f - 3.4793f * inv_eta + 6.75335f * inv_eta_2 - 7.80989f * inv_eta_3 + 4.98554f * inv_eta_4 - 1.36881f * inv_eta_5;
	}
}

vec3 CosWeight::Sample(const vec3& N, const vec2& sample) {
	float x1 = sample.x;
	float x2 = sample.y;

	float x = cos(2.0f * PI * x2) * sqrt(x1);
	float y = sin(2.0f * PI * x2) * sqrt(x1);
	float z = sqrt(1.0f - x1);
	vec3 L(x, y, z);
	L = ToWorld(L, N);

	return L;
}

float GGX::GeometrySmith_1(const vec3& V, const vec3& H, const vec3& N, float alpha_u, float alpha_v) {
	float cos_v_n = dot(V, N);

	if (cos_v_n * dot(V, H) <= 0.0f) {
		return 0.0f;
	}

	if (abs(cos_v_n - 1.0f) < EPS) {
		return 1.0f;
	}

	if (abs(alpha_u - alpha_v) < EPS) {
		float cos_v_n_2 = sqr(cos_v_n),
			tan_v_n_2 = (1.0f - cos_v_n_2) / cos_v_n_2,
			alpha_2 = alpha_u * alpha_u;

		return 2.0f / (1.0f + sqrt(1.0f + alpha_2 * tan_v_n_2));
	}
	else {
		vec3 dir = ToLocal(V, N);
		float xy_alpha_2 = sqr(alpha_u * dir.x) + sqr(alpha_v * dir.y),
			tan_v_n_alpha_2 = xy_alpha_2 / sqr(dir.z);

		return 2.0f / (1.0f + sqrt(1.0f + tan_v_n_alpha_2));
	}
}

float GGX::DistributionGGX(const vec3& H, const vec3& N, float alpha_u, float alpha_v) {
	float cos_theta = dot(H, N);
	if (cos_theta <= 0.0f) {
		return 0.0f;
	}
	float cos_theta_2 = sqr(cos_theta),
		tan_theta_2 = (1.0f - cos_theta_2) / cos_theta_2,
		alpha_2 = alpha_u * alpha_v;
	if (abs(alpha_u - alpha_v) < EPS) {
		return alpha_2 / (PI * pow(cos_theta, 3.0f) * sqr(alpha_2 + tan_theta_2));
	}
	else {
		vec3 dir = ToLocal(H, N);

		return cos_theta / (PI * alpha_2 * sqr(sqr(dir.x / alpha_u) + sqr(dir.y / alpha_v) + sqr(dir.z)));
	}
}

float GGX::DistributionVisibleGGX(const vec3& V, const vec3& H, const vec3& N, float alpha_u, float alpha_v) {
	return GeometrySmith_1(V, H, N, alpha_u, alpha_v) * dot(V, H) * DistributionGGX(H, N, alpha_u, alpha_v) / dot(N, V);
}

vec3 GGX::Sample(const vec3& N, float alpha_u, float alpha_v, const vec2& sample) {
	float sin_phi = 0.0f, cos_phi = 0.0f, alpha_2 = 0.0f;
	if ((alpha_u - alpha_v) < EPS) {
		float phi = 2.0f * PI * sample.y;
		cos_phi = cos(phi);
		sin_phi = sin(phi);
		alpha_2 = alpha_u * alpha_u;
	}
	else {
		float phi = atan(alpha_v / alpha_u * tan(PI + 2.0f * PI * sample.y)) + PI * floor(2.0f * sample.y + 0.5f);
		cos_phi = cos(phi);
		sin_phi = sin(phi);
		alpha_2 = 1.0f / (sqr(cos_phi / alpha_u) + sqr(sin_phi / alpha_v));
	}
	float tan_theta_2 = alpha_2 * sample.x / (1.0f - sample.x),
		cos_theta = 1.0f / sqrt(1.0f + tan_theta_2),
		sin_theta = sqrt(1.0f - cos_theta * cos_theta);
	vec3 H = ToWorld({ sin_theta * cos_phi, sin_theta * sin_phi, cos_theta }, N);

	return H;
}

vec3 GGX::SampleVisible(const vec3& N, const vec3& Ve, float alpha_u, float alpha_v, const vec2& sample) {
	vec3 V = ToLocal(Ve, N);
	vec3 Vh = normalize(vec3(alpha_u * V.x, alpha_v * V.y, V.z));

	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	float len2 = pow2(Vh.x) + pow2(Vh.y);
	vec3 T1 = len2 > 0.0f ? vec3(-Vh.y, Vh.x, 0.0f) * glm::inversesqrt(len2) : vec3(1.0f, 0.0f, 0.0f);
	vec3 T2 = glm::cross(Vh, T1);

	// Section 4.2: parameterization of the projected area
	float u = sample.x;
	float v = sample.y;
	float r = std::sqrt(u);
	float phi = v * 2.0f * PI;
	float t1 = r * std::cos(phi);
	float t2 = r * std::sin(phi);
	float s = 0.5f * (1.0f + Vh.z);
	t2 = (1.0f - s) * std::sqrt(1.0f - pow2(t1)) + s * t2;

	// Section 4.3: reprojection onto hemisphere
	vec3 Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0f, 1.0f - pow2(t1) - pow2(t2))) * Vh;

	// Section 3.4: transforming the normal back to the ellipsoid configuration
	vec3 H = normalize(vec3(alpha_u * Nh.x, alpha_v * Nh.y, std::max(0.0f, Nh.z)));

	return ToWorld(H, N);
}

float GTR1::DistributionGTR1(float NdotH, float a) {
	NdotH = abs(NdotH);
	if (a >= 1.0f) {
		return INV_PI;
	}
	float a2 = a * a;
	float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;

	return (a2 - 1.0f) / (PI * log(a2) * t);
}

vec3 GTR1::Sample(vec3 V, vec3 N, float alpha, vec2 sample) {
	float phi_h = 2.0f * PI * sample.x;
	float sin_phi_h = sin(phi_h);
	float cos_phi_h = cos(phi_h);

	float cos_theta_h = sqrt((1.0f - pow(alpha * alpha, 1.0f - sample.y)) / (1.0f - alpha * alpha));
	float sin_theta_h = sqrt(std::max(0.0f, 1.0f - cos_theta_h * cos_theta_h));

	//采样 "微平面" 的法向量 作为镜面反射的半角向量h 
	vec3 H = vec3(sin_theta_h * cos_phi_h, sin_theta_h * sin_phi_h, cos_theta_h);
	H = ToWorld(H, N);   //投影到真正的法向半球

	return H;
}

KullaConty::KullaConty(float (*G1)(const vec3& V, const vec3& H, const vec3& N, float alpha_u, float alpha_v),
	vec3(*Sample)(const vec3& N, float alpha_u, float alpha_v, const vec2& sample)) {
	vec3 normal(0.0f, 0.0f, 1.0f);

	vec3 h(0.0f, 1.0f, 0.0f);

	//粗糙度变化
	for (int k = 0; k < kLutResolution; k++) {
		float roughness = kStep * (k + 0.5f);
		//预计算光线出射方向与法线方向夹角的余弦从0到1的一系列反照率
		for (int j = 0; j < kLutResolution; j++) {
			float cos_theta_o = kStep * (j + 0.5f);
			vec3 wo = { sqrt(1.0f - sqr(cos_theta_o)), 0.0f, cos_theta_o };
			albedo_lut[k][j] = 0.0f;
			for (int i = 0; i < kSampleCount; i++) {
				h = Sample(normal, roughness, roughness, Hammersley(i + 1, kSampleCount + 1));
				vec3 wi = reflect(-wo, h);
				float G = G1(wi, h, normal, roughness, roughness) * G1(wo, h, normal, roughness, roughness),
					cos_m_o = std::max(dot(wo, h), 0.0f),
					cos_m_n = std::max(dot(normal, h), 0.0f);
				//重要性采样的微表面模型BSDF，并且菲涅尔项置为1（或0）
				albedo_lut[k][j] += (cos_m_o * G / (cos_theta_o * cos_m_n));
			}
			albedo_lut[k][j] = std::max(kSampleCountInv * albedo_lut[k][j], 0.0f);
			albedo_lut[k][j] = std::min(albedo_lut[k][j], 1.0f);
		}
	}

	albedo_avg_lut.fill(0.0f);
	//积分，计算平均反照率
	for (int k = 0; k < kLutResolution; k++) {
		float roughness = kStep * (k + 0.5f);
		for (int j = 0; j < kLutResolution; j++) {
			float avg_tmp = 0.0f,
				cos_theta_o = kStep * (j + 0.5f);
			vec3 wo = { sqrt(1.0f - sqr(cos_theta_o)), 0.0f, cos_theta_o };
			for (int i = 0; i < kSampleCount; i++) {
				h = Sample(normal, roughness, roughness, Hammersley(i + 1, kSampleCount + 1));
				vec3 wi = reflect(-wo, h);
				float cos_theta_i = std::max(dot(wi, normal), 0.0f);
				avg_tmp += (albedo_lut[k][j] * cos_theta_i);
			}
			albedo_avg_lut[k] += (avg_tmp * 2.0f * kSampleCountInv);
		}
		albedo_avg_lut[k] *= kStep;
	}
}

float KullaConty::Albedo(float cos_theta, float roughness) const {
	float offset = sqr(roughness) * kLutResolution;
	auto offset_int = static_cast<int>(offset);

	float offset2 = cos_theta * kLutResolution;
	auto offset2_int = static_cast<int>(offset2);
	
	if (offset_int < 0) {
//		cout << offset_int << endl;
		offset_int = 0;
	}
	if (offset2_int < 0) {
//		cout << offset2_int << endl;
		offset2_int = 0;
	}

	if (offset_int >= kLutResolution - 1) {
		if (offset2_int >= kLutResolution - 1) {
			return albedo_lut.back().back();
		}
		else {
			return glm::mix(albedo_lut.back()[offset2_int], albedo_lut.back()[offset2_int + 1], offset2 - offset2_int);
		}
	}
	else {
		if (offset2_int >= kLutResolution - 1) {
			return glm::mix(albedo_lut[offset_int].back(), albedo_lut[offset_int + 1].back(), offset - offset_int);
		}
		else {
			float albedo1 = glm::mix(albedo_lut[offset_int][offset2_int], albedo_lut[offset_int + 1][offset2_int], offset - offset_int);
			float albedo2 = glm::mix(albedo_lut[offset_int][offset2_int + 1], albedo_lut[offset_int + 1][offset2_int + 1], offset - offset_int);
//			cout << albedo1 << " " << albedo2 << endl;

			return glm::mix(albedo1, albedo2, offset2 - offset2_int);
		}
	}
}

float KullaConty::AverageAlbedo(float roughness) const {
	float offset = sqr(roughness) * kLutResolution;
	auto offset_int = static_cast<int>(offset);

	if (offset_int < 0) {
		offset_int = 0;
	}

	if (offset_int >= kLutResolution - 1) {
		return albedo_avg_lut.back();
	}
	else {
		return glm::mix(albedo_avg_lut[offset_int], albedo_avg_lut[offset_int + 1], offset - offset_int);
	}
}

vec3 KullaConty::EvalMultipleScatter(float NdotL, float NdotV, float roughness, vec3 F_avg) {
	vec3 f_add = F_avg * F_avg * AverageAlbedo(roughness) / (vec3(1.0f) - F_avg * (1.0f - AverageAlbedo(roughness)));

	float albedo_i = Albedo(abs(NdotL), roughness),
		albedo_o = Albedo(abs(NdotV), roughness),
		f_ms = (1.0f - albedo_o) * (1.0f - albedo_i) / (PI * (1.0f - AverageAlbedo(roughness)));

	return f_ms * f_add;
}

BsdfSample DiffuseLight::Sample(const vec3& V, const IntersectionInfo& info) {
	return { vec3(0.0f), 0.0f };
}

EvalInfo DiffuseLight::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	return { vec3(0.0f), 0.0f };
}

vec3 DiffuseLight::Emitted(const IntersectionInfo& info) {
	return emittedTexture->Value(info.uv, info.position);
}

vec3 DiffuseLight::GetAlbedo(const IntersectionInfo& info) {
	return emittedTexture->Value(info.uv, info.position);
}

BsdfSample SmoothDiffuse::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;

// 	float x1 = RandomFloat();
// 	float x2 = RandomFloat();
	vec2 sample(RandomFloat(), RandomFloat());
//	cout << to_string(r2) << endl;
//	cout << "fr:" << info.frameCounter << " bo" << info.bounceCounter << endl;
	vec3 L = CosWeight::Sample(info.normal, sample);

	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	float pdf = NdotL * INV_PI;

	return { L, pdf };
}

EvalInfo SmoothDiffuse::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 albedo = albedoTexture->Value(info.uv, info.position);

	float NdotL = dot(info.normal, L);
	float NdotV = dot(info.normal, V);
	if (NdotV < 0.0f || NdotL < 0.0f) {
		return { vec3(0.0f), 0.0f };
	}
	vec3 brdf = albedo * INV_PI;
	float pdf = NdotL * INV_PI;

	return { brdf, NdotL, pdf };
}

vec3 SmoothDiffuse::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample SmoothConductor::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}
	vec3 L = reflect(-V, N);
	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	return { L , 1.0f };
}

EvalInfo SmoothConductor::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}
	vec3 brdf = albedoTexture->Value(info.uv, info.position) * Fresnel::FresnelConductor(L, N, eta, k);

	return { brdf, 1.0f, 1.0f };
}

bool SmoothConductor::IsDelta() const noexcept {
	return true;
}

vec3 SmoothConductor::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample SmoothDielectric::Sample(const vec3& V, const IntersectionInfo& info) {
	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);
	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	float F = Fresnel::FresnelDielectric(V, N, etai_over_etat);
	if (RandomFloat() < F) {
		vec3 L = reflect(-V, N);
		if (dot(N, L) < 0.0f) {
			return BsdfSampleError();
		}

		return { L, F };
	}
	else {
		vec3 L = refract(-V, N, etai_over_etat);

		//采样不合法，舍去
		if (dot(N, L) * dot(N, V) > 0.0f) {
//			cout << "不合法" << endl;
			return BsdfSampleError();
		}

		return { L, 1.0f - F };
	}
}

EvalInfo SmoothDielectric::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);
	vec3 albedo = albedoTexture->Value(info.uv, info.position);

	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	vec3 bsdf;
	float F = Fresnel::FresnelDielectric(V, N, etai_over_etat);

	bool isReflect = dot(N, L) * dot(N, V) > 0.0f;
	if (isReflect) {
		bsdf = albedo * F;

		return { bsdf, 1.0f, F };
	}
	else {
		bsdf = albedo * (1.0f - F) * sqr(1.0f / etai_over_etat);

		return { bsdf, 1.0f, 1.0f - F };
	}
}

bool SmoothDielectric::IsDelta() const noexcept {
	return true;
}

vec3 SmoothDielectric::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample SmoothPlastic::Sample(const vec3& V, const IntersectionInfo& info) {
	const vec3 kd = diffuseTexture->Value(info.uv, info.position),
		ks = specularTexture->Value(info.uv, info.position);
	const float d_sum = kd.r + kd.g + kd.b,
		s_sum = ks.r + ks.g + ks.b;

	vec3 N = info.normal;
	//	bool add_specular = true;//生成的光线方向是否在镜面反射波瓣之中
	float Fo = Fresnel::FresnelDielectric(V, N, eta_inv),//出射菲涅尔项
		Fi = Fo,//入射菲涅尔项
		specular_sampling_weight = s_sum / (s_sum + d_sum),//抽样镜面反射的权重
		pdf_specular = Fi * specular_sampling_weight,//抽样镜面反射分量的概率
		pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);//抽样漫反射分量的概率
	pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

	vec2 sample(RandomFloat(), RandomFloat());

	float pdf;
	vec3 L;
	if (RandomFloat() < pdf_specular) { //从镜面反射分量抽样光线方向
		L = reflect(-V, N);

		float NdotL = dot(info.normal, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}

		pdf = pdf_specular + (1.0f - pdf_specular) * NdotL * INV_PI;
	}
	else { //从漫反射分量抽样光线方向
		L = CosWeight::Sample(info.normal, sample);

		float NdotL = dot(info.normal, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}

		pdf = pdf_specular + (1.0f - pdf_specular) * NdotL * INV_PI;
	}

	return { L, pdf };
}

EvalInfo SmoothPlastic::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	const vec3 kd = diffuseTexture->Value(info.uv, info.position),
		ks = specularTexture->Value(info.uv, info.position);
	const float d_sum = kd.r + kd.g + kd.b,
		s_sum = ks.r + ks.g + ks.b;

	bool sampled_specular = false;//是否抽样到了镜面反射分量
	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	float NdotV = dot(N, V);
	float NdotL = dot(N, L);
	if (NdotV <= 0.0f || NdotL <= 0.0f) {
		return { vec3(0.0f), NdotL };
	}

	float Fo = Fresnel::FresnelDielectric(V, N, eta_inv),//出射菲涅尔项
		Fi = Fresnel::FresnelDielectric(L, N, eta_inv),//入射菲涅尔项
		specular_sampling_weight = s_sum / (s_sum + d_sum),//抽样镜面反射的权重
		pdf_specular = Fi * specular_sampling_weight,//抽样镜面反射分量的概率
		pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);//抽样漫反射分量的概率
	pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

	float pdf = (1.0f - pdf_specular) * std::max(0.0f, dot(N, V)) * INV_PI;
	if (SameDirection(L, reflect(-V, N))) { //如果出射方向位于镜面反射波瓣之内，则再加上镜面反射成分的概率
		sampled_specular = true;
		pdf += pdf_specular;
	}

	vec3 diffuse = kd, specular = ks;
	vec3 brdf;
	if (nonlinear) {
		brdf = diffuse / (1.0f - diffuse * fdr);
	}
	else {
		brdf = diffuse / (1.0f - fdr);
	}

	brdf *= (1.0f - Fi) * (1.0f - Fo) * INV_PI;
	if (sampled_specular) {
		brdf *= NdotL;
		brdf += Fi * specular;
		NdotL = 1.0f;
	}

	return { brdf, NdotL, pdf };
}

vec3 SmoothPlastic::GetAlbedo(const IntersectionInfo& info) {
	return diffuseTexture->Value(info.uv, info.position);
}

BsdfSample RoughDiffuse::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;

// 	float x1 = RandomFloat();
// 	float x2 = RandomFloat();
	vec2 sample(RandomFloat(), RandomFloat());

	vec3 L = CosWeight::Sample(info.normal, sample);

	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	float pdf = NdotL * INV_PI;

	return { L, pdf };
}

EvalInfo RoughDiffuse::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 H = normalize(V + L);
	vec3 n = info.normal;

	float VdotH = dot(V, H);
	float NdotV = dot(n, V);
	float NdotL = dot(n, L);
	if (NdotV < 0.0f || NdotL < 0.0f) {
		return { vec3(0.0f), 0.0f };
	}

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;

	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float VdotL = 2.0f * VdotH * VdotH - 1.0f;
	float Cosri = VdotL - NdotV * NdotL;
	float C1 = 1.0f - 0.5f * s2 / (s2 + 0.33f);
	float C2 = 0.45f * s2 / (s2 + 0.09f) * Cosri * (Cosri >= 0.0f ? (std::max(NdotL, NdotV)) : 1.0f);

	vec3 brdf = albedo / PI * (C1 + C2) * (1.0f + roughness * 0.5f);
	float pdf = NdotL * INV_PI;

	return { brdf, NdotL, pdf };
}

vec3 RoughDiffuse::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample RoughConductor::Sample(const vec3& V, const IntersectionInfo& info) {
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float aniso = anisotropyTexture->Value(info.uv, info.position).r;
	float alpha_u = sqr(roughness) * (1.0f + aniso);
	float alpha_v = sqr(roughness) * (1.0f - aniso);

	vec3 N = info.normal;

	vec2 sample(RandomFloat(), RandomFloat());

//	vec3 H = GGX::Sample(N, alpha_u, alpha_v, xy);
//	float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);
	vec3 H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sample);
	float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
	float pdf = Dv * abs(1.0f / (4.0f * dot(V, H)));

	vec3 L = reflect(-V, H);
	float NdotL = dot(N, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	return { L, pdf };
}

EvalInfo RoughConductor::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float aniso = anisotropyTexture->Value(info.uv, info.position).r;
	float alpha_u = sqr(roughness) * (1.0f + aniso);
	float alpha_v = sqr(roughness) * (1.0f - aniso);

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	vec3 N = info.normal;
	vec3 H = normalize(V + L);
	float NdotV = dot(N, V);
	float NdotL = dot(N, L);

	if (NdotV <= 0.0f || NdotL <= 0.0f) {
		return { vec3(0.0f), NdotL };
	}

	vec3 F = Fresnel::FresnelConductor(V, H, eta, k);
	float G = GGX::GeometrySmith_1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith_1(L, H, N, alpha_u, alpha_v);
	float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);
	vec3 brdf = F * D * G / (4.0f * NdotV * NdotL);
	vec3 mult = kulla_conty->EvalMultipleScatter(NdotL, NdotV, roughness, F_avg);
	brdf += mult;
//	cout << mult.x << " " << mult.y << " " << mult.z << endl;
	brdf *= albedo;

	float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
	float pdf = Dv * abs(1.0f / (4.0f * dot(V, H)));

	return { brdf, NdotL, pdf };
}

vec3 RoughConductor::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample RoughDielectric::Sample(const vec3& V, const IntersectionInfo& info) {
	float etai_over_etat = info.frontFace ? (eta_inv) : (eta);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float aniso = anisotropyTexture->Value(info.uv, info.position).r;
	float alpha_u = sqr(roughness) * (1.0f + aniso);
	float alpha_v = sqr(roughness) * (1.0f - aniso);

	vec3 N = info.normal;

	vec2 sample(RandomFloat(), RandomFloat());

//	vec3 H = GGX::Sample(N, alpha_u, alpha_v, xy);
//	float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);
	vec3 H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sample);
	float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
	float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);

	float pdf;
	vec3 L;
	if (RandomFloat() < F) {
		L = reflect(-V, H);

		if (dot(N, L) < 0.0f) {
			return BsdfSampleError();
		}

		float dwh_dwi = abs(1.0f / (4.0f * dot(V, H)));
		pdf = F * Dv * dwh_dwi;
	}
	else {
		L = refract(-V, H, etai_over_etat);

		//折射不可能在同侧，舍去
		if (dot(N, L) * dot(N, V) > 0.0f) {
			return BsdfSampleError();
		}

		float HdotV = dot(H, V);
		float HdotL = dot(H, L);
		float sqrtDenom = etai_over_etat * HdotV + HdotL;
		float dwh_dwi = abs(HdotL) / sqr(sqrtDenom);
		pdf = (1.0f - F) * Dv * dwh_dwi;
	}

	return { L, pdf };
}

EvalInfo RoughDielectric::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	bool inside = info.frontFace;
	float etai_over_etat = inside ? (eta_inv) : (eta);
	float ratio = inside ? ratio_t_inv : ratio_t;
	float F_a = inside ? F_avg_inv : F_avg;
	// 	cout << etai_over_etat << " " << ratio << " " << F_a << endl;
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float aniso = anisotropyTexture->Value(info.uv, info.position).r;
	float alpha_u = sqr(roughness) * (1.0f + aniso);
	float alpha_v = sqr(roughness) * (1.0f - aniso);

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	vec3 N = info.normal;
	vec3 H;
	float NdotV = abs(dot(N, V));
	float NdotL = abs(dot(N, L));

	vec3 bsdf;
	float pdf;
	float dwh_dwi;
	bool isReflect = dot(N, L) * dot(N, V) > 0.0f;
	if (isReflect) {
		H = normalize(V + L);
		float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);
		float G = GGX::GeometrySmith_1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith_1(L, H, N, alpha_u, alpha_v);
		float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);

		vec3 mult = (1.0f - ratio) * kulla_conty->EvalMultipleScatter(NdotL, NdotV, roughness, vec3(F_a));
		bsdf = vec3(F) * D * G / (4.0f * NdotV * NdotL);
		bsdf += mult;
		dwh_dwi = abs(1.0f / (4.0f * dot(V, H)));
		float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
		pdf = Dv * dwh_dwi * (isReflect ? F : 1 - F);
// 		cout << "refl";
// 		cout << mult.x << " " << mult.y << " " << mult.z << endl;
	}
	else {
		H = -normalize(etai_over_etat * V + L);
		if (dot(N, H) < 0.0f) {
			H = -H;
		}
		//		cout << "eval" << H.x << " " << H.y << " " << H.z << endl;
		float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);
		float G = GGX::GeometrySmith_1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith_1(L, H, N, alpha_u, alpha_v);
		float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);
		float HdotV = dot(H, V);
		float HdotL = dot(H, L);
		float sqrtDenom = etai_over_etat * HdotV + HdotL;
		float factor = abs(HdotL * HdotV / (NdotL * NdotV));

		vec3 mult = ratio * kulla_conty->EvalMultipleScatter(NdotL, NdotV, roughness, vec3(F_a));
		bsdf = vec3(1.0f - F) * D * G * factor / sqr(sqrtDenom);
		bsdf += mult;
		dwh_dwi = abs(HdotL) / sqr(sqrtDenom);
		float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
		pdf = Dv * dwh_dwi * (isReflect ? F : 1 - F);
// 		cout << "refff";
// 		cout << mult.x << " " << mult.y << " " << mult.z << endl;
	}

	bsdf *= albedo;
	 
	return { bsdf, NdotL, pdf };
}

vec3 RoughDielectric::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample RoughPlastic::Sample(const vec3& V, const IntersectionInfo& info) {
	const vec3 kd = diffuseTexture->Value(info.uv, info.position),
		ks = specularTexture->Value(info.uv, info.position);
	const float d_sum = kd.r + kd.g + kd.b,
		s_sum = ks.r + ks.g + ks.b;

	vec3 N = info.normal;
	//	bool add_specular = true;//生成的光线方向是否在镜面反射波瓣之中
	float Fo = Fresnel::FresnelDielectric(V, N, eta_inv),//出射菲涅尔项
		Fi = Fo,//入射菲涅尔项
		specular_sampling_weight = s_sum / (s_sum + d_sum),//抽样镜面反射的权重
		pdf_specular = Fi * specular_sampling_weight,//抽样镜面反射分量的概率
		pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);//抽样漫反射分量的概率
	pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float aniso = anisotropyTexture->Value(info.uv, info.position).r;
	float alpha_u = sqr(roughness) * (1.0f + aniso);
	float alpha_v = sqr(roughness) * (1.0f - aniso);

	vec2 sample(RandomFloat(), RandomFloat());

	float pdf;
	vec3 L;
	if (RandomFloat() < pdf_specular) { //从镜面反射分量抽样光线方向
//	    vec3 H = GGX::Sample(N, alpha_u, alpha_v, xy);
//	    float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);
		vec3 H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sample);
		float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
		L = reflect(-V, H);

		float NdotL = dot(N, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}

//		pdf = D * abs(dot(N, H) / (4.0f * dot(V, H)));
		pdf = pdf_specular * Dv * abs(1.0f / (4.0f * dot(V, H))) + (1.0f - pdf_specular) * NdotL * INV_PI;
	}
	else { //从漫反射分量抽样光线方向
		L = CosWeight::Sample(N, sample);

		float NdotL = dot(N, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}

		vec3 H = normalize(V + L);
		float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
//		pdf = NdotL * INV_PI;
		pdf = pdf_specular * Dv * abs(1.0f / (4.0f * dot(V, H))) + (1.0f - pdf_specular) * NdotL * INV_PI;
	}

	return { L, pdf };
}

EvalInfo RoughPlastic::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	const vec3 kd = diffuseTexture->Value(info.uv, info.position),
		ks = specularTexture->Value(info.uv, info.position);
	const float d_sum = kd.r + kd.g + kd.b,
		s_sum = ks.r + ks.g + ks.b;

	vec3 N = info.normal;
	vec3 H = normalize(V + L);

	float NdotV = dot(N, V);
	float NdotL = dot(N, L);

	if (NdotV <= 0.0f || NdotL <= 0.0f) {
		return { vec3(0.0f), NdotL, 1.0f };
	}

	float Fo = Fresnel::FresnelDielectric(V, N, eta_inv),//出射菲涅尔项
		Fi = Fresnel::FresnelDielectric(L, N, eta_inv),//入射菲涅尔项
		specular_sampling_weight = s_sum / (s_sum + d_sum),//抽样镜面反射的权重
		pdf_specular = Fi * specular_sampling_weight,//抽样镜面反射分量的概率
		pdf_diffuse = (1.0f - Fi) * (1.0f - specular_sampling_weight);//抽样漫反射分量的概率
	pdf_specular = pdf_specular / (pdf_specular + pdf_diffuse);

	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float aniso = anisotropyTexture->Value(info.uv, info.position).r;
	float alpha_u = sqr(roughness) * (1.0f + aniso);
	float alpha_v = sqr(roughness) * (1.0f - aniso);

	vec3 diffuse = kd, specular = ks;
	vec3 brdf;
	if (nonlinear) {
		brdf = diffuse / (1.0f - diffuse * fdr);
	}
	else {
		brdf = diffuse / (1.0f - fdr);
	}
	brdf *= (1.0f - Fi) * (1.0f - Fo) * INV_PI;
//	cout << to_string(brdf) << endl;
	float F = Fi;
	float G = GGX::GeometrySmith_1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith_1(L, H, N, alpha_u, alpha_v);
	float D = GGX::DistributionGGX(H, N, alpha_u, alpha_v);

	vec3 specular_brdf = vec3(F) * D * G / (4.0f * NdotL * NdotV);
	vec3 mult = kulla_conty->EvalMultipleScatter(NdotL, NdotV, roughness, vec3(fdr));
	specular_brdf += mult;

	brdf += specular_brdf * specular;

	float Dv = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
	float pdf = pdf_specular * Dv * abs(1.0f / (4.0f * dot(V, H))) + (1.0f - pdf_specular) * NdotL * INV_PI;

	return { brdf, NdotL, pdf };
}

vec3 RoughPlastic::GetAlbedo(const IntersectionInfo& info) {
	return diffuseTexture->Value(info.uv, info.position);
}

BsdfSample ClearcoatedConductor::Sample(const vec3& V, const IntersectionInfo& info) {
	float alpha_u = sqr(roughnessTexture_u->Value(info.uv, info.position).r);
	float alpha_v = sqr(roughnessTexture_v->Value(info.uv, info.position).r);

	vec3 N = info.normal;
	float NdotV = dot(V, N); // 出射光线方向和宏观表面法线方向夹角的余弦
	float weight_coat = clear_coat * Fresnel::FresnelDielectric(V, N, 1.0f / 1.5f);

	vec2 sample(RandomFloat(), RandomFloat());

	if (RandomFloat() < weight_coat) {
//	    vec3 H = GGX::Sample(N, alpha_u, alpha_v, xy);
		vec3 H = GGX::SampleVisible(N, V, alpha_u, alpha_v, sample);
		float Dv_coat = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
		vec3 L = reflect(-V, H);

		float NdotV = dot(N, V);
		float NdotL = dot(N, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}

		const float F_coat = Fresnel::FresnelDielectric(L, H, 1.0f / 1.5f);
		weight_coat = clear_coat * F_coat;

		float pdf_coat = Dv_coat * std::abs(1.0f / (4.0f * dot(V, H)));
		EvalInfo con_info = conductor->Eval(V, L, info);

		float pdf_nested = con_info.bsdf_pdf;

		float pdf = pdf_nested * (1.0f - weight_coat) + weight_coat * pdf_coat;

		return { L, pdf };
	}
	else {
		BsdfSample con_sample = conductor->Sample(V, info);
		vec3 L = con_sample.bsdf_dir;

		float NdotL = dot(N, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}

		EvalInfo con_info = conductor->Eval(V, L, info);
		float pdf_nested = con_info.bsdf_pdf;

		vec3 H = normalize(V + L);
		const float F_coat = Fresnel::FresnelDielectric(L, H, 1.0f / 1.5f);
		weight_coat = clear_coat * F_coat;
		float Dv_coat = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
		float pdf_coat = Dv_coat * std::abs(1.0f / (4.0f * dot(V, H)));

		float pdf = pdf_nested * (1.0f - weight_coat) + weight_coat * pdf_coat;

		return { L, pdf };
	}
}
EvalInfo ClearcoatedConductor::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	float alpha_u = sqr(roughnessTexture_u->Value(info.uv, info.position).r);
	float alpha_v = sqr(roughnessTexture_v->Value(info.uv, info.position).r);

	vec3 N = info.normal;
	float NdotV = dot(N, V);
	float NdotL = dot(N, L);
	if (NdotV < 0.0f) {
		return { vec3(0.0f), NdotL };
	}
	
	vec3 H = normalize(V + L);
	EvalInfo con_info = conductor->Eval(V, L, info);
	vec3 con_brdf = con_info.bsdf;
	float pdf_nested = con_info.bsdf_pdf;

	const float F_coat = Fresnel::FresnelDielectric(L, H, 1.0f / 1.5f); // 菲涅尔项
	float weight_coat = clear_coat * F_coat;
	float D_coat = GGX::DistributionGGX(H, N, alpha_u, alpha_v);

	float pdf_coat = 0.0f;
	vec3 coat_brdf(0.0f);
	if (D_coat > 0.0f) {
		float Dv_coat = GGX::DistributionVisibleGGX(V, H, N, alpha_u, alpha_v);
		pdf_coat = Dv_coat * std::abs(1.0f / (4.0f * dot(V, H)));
		float G_coat = GGX::GeometrySmith_1(V, H, N, alpha_u, alpha_v) * GGX::GeometrySmith_1(L, H, N, alpha_u, alpha_v);
		coat_brdf = vec3(F_coat * D_coat * G_coat / abs(4.0f * NdotL * NdotV));
	}
	float pdf = pdf_nested * (1.0f - weight_coat) + weight_coat * pdf_coat;
	vec3 brdf = con_brdf * (1.0f - weight_coat) + weight_coat * coat_brdf;

	return { brdf, NdotL, pdf };
}

vec3 ClearcoatedConductor::GetAlbedo(const IntersectionInfo& info) {
	return conductor->GetAlbedo(info);
}

BsdfSample ThinDielectric::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	float pdf;
	vec3 L;
	float F = Fresnel::FresnelDielectric(V, N, eta_inv);
	if (F < 1.0f) {
		F *= 2.0f / (1.0f + F);
	}
	if (RandomFloat() < F) {
		L = reflect(-V, N);
		if (dot(N, L) < 0.0f) {
			return BsdfSampleError();
		}

		pdf = F;
	}
	else {
		L = -V;
		pdf = 1.0f - F;

		if (dot(N, L) * dot(N, V) > 0.0f) {
			return BsdfSampleError();
		}
	}

	return { L, pdf };
}

EvalInfo ThinDielectric::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 albedo = albedoTexture->Value(info.uv, info.normal);
	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	float F = Fresnel::FresnelDielectric(V, N, eta_inv);
	if (F < 1.0f) {
		F *= 2.0f / (1.0f + F);
	}

	vec3 bsdf;
	if (dot(N, L) * dot(N, V) > 0.0f) {
		bsdf = albedo * F;

		return { bsdf , 1.0f, F };
	}
	else {
		bsdf = albedo * (1.0f - F);

		return { bsdf , 1.0f, 1.0f - F };
	}
}

bool ThinDielectric::IsDelta() const noexcept {
	return true;
}

vec3 ThinDielectric::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample MetalWorkflow::Sample(const vec3& V, const IntersectionInfo& info) {
	float roughness;
	float metallic;
	if (metallicRoughnessTexture == NULL) {
		roughness = roughnessTexture->Value(info.uv, info.position).r;
		metallic = metallicTexture->Value(info.uv, info.position).r;
	}
	if(metallicTexture == NULL && roughnessTexture == NULL) {
		roughness = metallicRoughnessTexture->Value(info.uv, info.position).g;
		metallic = metallicRoughnessTexture->Value(info.uv, info.position).b;
	}

	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	float metallic_brdf = metallic;
	float dieletric_brdf = (1.0f - metallic);
	float diffuse = dieletric_brdf;
	float specular = metallic_brdf + dieletric_brdf;
	float deom = diffuse + specular;

	float p_diffuse = diffuse / deom;
	float p_specular = specular / deom;

	vec2 sample(RandomFloat(), RandomFloat());

	float t = RandomFloat();
	vec3 L;
	if (t <= p_diffuse) {
		//diffuse
		vec3 L = CosWeight::Sample(info.normal, sample);

		float NdotL = dot(info.normal, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}
	}
	else if (t <= p_diffuse + p_specular) {
		vec3 H = GGX::SampleVisible(N, V, roughness, roughness, sample);
		L = reflect(-V, H);

		float NdotL = dot(info.normal, L);
		if (NdotL < 0.0f) {
			return BsdfSampleError();
		}
	}

	float diffuse_pdf = dot(N, L) * INV_PI;

	vec3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float VdotH = dot(V, H);
	float Dv = GGX::DistributionVisibleGGX(V, H, N, roughness, roughness);

	float specular_pdf = Dv * abs(1.0f / (4.0f * VdotH));

	float pdf = p_diffuse * diffuse_pdf + p_specular * specular_pdf;

	return { L, pdf };
}

EvalInfo MetalWorkflow::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	float roughness;
	float metallic;
	if (metallicRoughnessTexture == NULL) {
		roughness = roughnessTexture->Value(info.uv, info.position).r;
		metallic = metallicTexture->Value(info.uv, info.position).r;
	}
	if (metallicTexture == NULL && roughnessTexture == NULL) {
		roughness = metallicRoughnessTexture->Value(info.uv, info.position).g;
		metallic = metallicRoughnessTexture->Value(info.uv, info.position).b;
	}
	vec3 albedo = albedoTexture->Value(info.uv, info.position);

	float metallic_brdf = metallic;
	float dieletric_brdf = (1.0f - metallic);
	float diffuse = dieletric_brdf;
	float specular = metallic_brdf + dieletric_brdf;
	float deom = diffuse + specular;

	float p_diffuse = diffuse / deom;
	float p_specular = specular / deom;

	vec3 N = info.normal;
	if (normalTexture != NULL) {
		vec3 tangentNormal = normalTexture->Value(info.uv, info.position);
		N = NormalFormTangentToWorld(info.normal, tangentNormal);
	}

	vec3 H = normalize(L + V);
	float NdotL = dot(N, L);
	float LdotH = dot(L, H);
	float NdotH = dot(N, H);
	float NdotV = dot(N, V);
	float VdotH = dot(V, H);

	if (NdotL < 0.0f || NdotV < 0.0f) {
		return { vec3(0.0f), NdotL };
	}

	vec3 F0 = mix(vec3(0.04f), albedo, metallic);
	vec3 F = Fresnel::FresnelSchlick(F0, VdotH);
	float D = GGX::DistributionGGX(H, N, roughness, roughness);
	float G = GGX::GeometrySmith_1(V, H, N, roughness, roughness) * GGX::GeometrySmith_1(L, H, N, roughness, roughness);
	vec3 specular_brdf = D * F * G / (4.0f * NdotL * NdotV);
	vec3 diffuse_brdf = albedo * INV_PI;

	vec3 brdf = p_diffuse * diffuse_brdf + p_specular * specular_brdf;
	float diffuse_pdf = dot(N, L) * INV_PI;
	float Dv = GGX::DistributionVisibleGGX(V, H, N, roughness, roughness);
	float specular_pdf = Dv * abs(1.0f / (4.0f * VdotH));
	float pdf = p_diffuse * diffuse_pdf + p_specular * specular_pdf;

	return { brdf, NdotL, pdf };
}

vec3 MetalWorkflow::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample DisneyDiffuse::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;

	vec2 sample(RandomFloat(), RandomFloat());

	vec3 L = CosWeight::Sample(info.normal, sample);

	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	float pdf = NdotL * INV_PI;

	return { L, pdf };
}
EvalInfo DisneyDiffuse::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 N = info.normal;
	vec3 H = normalize(V + L);
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float HdotL = dot(H, L);
	float HdotV = dot(H, V);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		return { vec3(0.0f), NdotL, 1.0f };
	}

	float pdf = NdotL * INV_PI;

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float subsurface = subsurfaceTexture->Value(info.uv, info.position).r;
	
	float FL = Fresnel::SchlickWeight(NdotL);
	float FV = Fresnel::SchlickWeight(NdotV);
	float FD90 = 0.5f + 2.0f * roughness * sqr(HdotV);
	float FD_L = 1.0f + (FD90 - 1.0f) * FL;
	float FD_V = 1.0f + (FD90 - 1.0f) * FV;

	vec3 brdf_diffuse = albedo * INV_PI * FD_L * FD_V;

	float FSS90 = roughness * sqr(HdotV);
	float FSS_L = 1.0f + (FSS90 - 1.0f) * FL;
	float FSS_V = 1.0f + (FSS90 - 1.0f) * FV;

	vec3 brdf_subsurface = 1.25f * albedo * INV_PI * (FSS_L * FSS_V * (1.0f / (NdotL + NdotV) - 0.5f) + 0.5f);

	vec3 brdf = (1.0f - subsurface) * brdf_diffuse + subsurface * brdf_subsurface;

	return { brdf, NdotL, pdf };
}

vec3 DisneyDiffuse::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample DisneyMetal::Sample(const vec3& V, const IntersectionInfo& info) {
	vec2 sample(RandomFloat(), RandomFloat());

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float anisotropic = anisotropicTexture->Value(info.uv, info.position).r;

	float aspect = sqrt(1.0f - 0.9f * anisotropic);
	float ax = std::max(0.0001f, sqr(roughness) / aspect);
	float ay = std::max(0.0001f, sqr(roughness) * aspect);

	vec3 N = info.normal;
	vec3 H = GGX::SampleVisible(N, V, ax, ay, sample);
	float Dv = GGX::DistributionVisibleGGX(V, H, N, ax, ay);

	vec3 L = reflect(-V, H);

	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	float pdf = Dv * abs(1.0f / (4.0f * dot(V, H)));

	return { L, pdf };
}
EvalInfo DisneyMetal::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 N = info.normal;
	vec3 H = normalize(V + L);
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float HdotL = dot(H, L);
	float HdotV = dot(H, V);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		return { vec3(0.0f), NdotL, 1.0f };
	}

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float anisotropic = anisotropicTexture->Value(info.uv, info.position).r;
	float metallic = metallicTexture->Value(info.uv, info.position).r;
	vec3 specular = specularTexture->Value(info.uv, info.position);
	vec3 specularTint = specularTintTexture->Value(info.uv, info.position);

	float aspect = sqrt(1.0f - 0.9f * anisotropic);
	float ax = std::max(0.0001f, sqr(roughness) / aspect);
	float ay = std::max(0.0001f, sqr(roughness) * aspect);

	float clum = Luminance(albedo);
	vec3 ctint = clum > 0.0f ? albedo / clum : vec3(1.0f);
	vec3 ks = (1.0f - specularTint) + specularTint * ctint;
	vec3 c0 = specular * (1.0f - metallic) * ks + metallic * albedo;
	vec3 FM = Fresnel::FresnelSchlick(c0, HdotV);

	float G = GGX::GeometrySmith_1(V, H, N, ax, ay) * GGX::GeometrySmith_1(L, H, N, ax, ay);
	float D = GGX::DistributionGGX(H, N, ax, ay);

	vec3 brdf = FM * D * G / (4.0f * NdotL * NdotV);

	float Dv = GGX::DistributionVisibleGGX(V, H, N, ax, ay);
	float pdf = Dv * abs(1.0f / (4.0f * dot(V, H)));

	return { brdf, NdotL, pdf };
}

vec3 DisneyMetal::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample DisneyClearcoat::Sample(const vec3& V, const IntersectionInfo& info) {
	vec2 sample(RandomFloat(), RandomFloat());

	float clearcoatGloss = clearcoatGlossTexture->Value(info.uv, info.position).r;
	float ag = glm::mix(0.1f, 0.001f, clearcoatGloss);

	vec3 N = info.normal;
	vec3 H = GTR1::Sample(V, N, ag, sample);
	float NdotH = dot(N, H);

	vec3 L = reflect(-V, H);

	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	float D = GTR1::DistributionGTR1(NdotH, ag);
	float pdf = D * abs(dot(N, H) / (4.0f * dot(V, H)));

	return { L, pdf };
}
EvalInfo DisneyClearcoat::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 N = info.normal;
	vec3 H = normalize(V + L);
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float HdotL = dot(H, L);
	float HdotV = dot(H, V);
	float NdotH = dot(N, H);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		return { vec3(0.0f), NdotL, 1.0f };
	}

	float clearcoatGloss = clearcoatGlossTexture->Value(info.uv, info.position).r;
	float ag = glm::mix(0.1f, 0.001f, clearcoatGloss);

	float R0 = (1.5f - 1.0f) / (1.5f + 1.0f);
	float F = Fresnel::FresnelSchlick(R0, HdotV);
	float G = GGX::GeometrySmith_1(V, H, N, 0.25f, 0.25f) * GGX::GeometrySmith_1(L, H, N, 0.25f, 0.25f);
	float D = GTR1::DistributionGTR1(NdotH, ag);

	vec3 brdf = vec3(F * D * G / (4.0f * NdotL * NdotV));
	float pdf = D * abs(dot(N, H) / (4.0f * dot(V, H)));

	return { brdf, NdotL, pdf };
}

vec3 DisneyClearcoat::GetAlbedo(const IntersectionInfo& info) {
	return vec3(1.0f);
}

BsdfSample DisneyGlass::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;

	vec2 sample(RandomFloat(), RandomFloat());

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float anisotropic = anisotropicTexture->Value(info.uv, info.position).r;

	float aspect = sqrt(1.0f - 0.9f * anisotropic);
	float ax = std::max(0.0001f, sqr(roughness) / aspect);
	float ay = std::max(0.0001f, sqr(roughness) * aspect);

	float etai_over_etat = info.frontFace ? (1.0f / eta) : (eta);
	vec3 H = GGX::SampleVisible(N, V, ax, ay, sample);
	float Dv = GGX::DistributionVisibleGGX(V, H, N, ax, ay);
	float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);

	float pdf;
	vec3 L;
	if (RandomFloat() < F) {
		L = reflect(-V, H);

		if (dot(N, L) < 0.0f) {
			return BsdfSampleError();
		}

		float dwh_dwi = abs(1.0f / (4.0f * dot(V, H)));
		pdf = F * Dv * dwh_dwi;
	}
	else {
		L = refract(-V, H, etai_over_etat);

		//折射不可能在同侧，舍去
		if (dot(N, L) * dot(N, V) > 0.0f) {
			return BsdfSampleError();
		}

		float HdotV = dot(H, V);
		float HdotL = dot(H, L);
		float sqrtDenom = etai_over_etat * HdotV + HdotL;
		float dwh_dwi = abs(HdotL) / sqr(sqrtDenom);
		pdf = (1.0f - F) * Dv * dwh_dwi;
	}

	return { L, pdf };
}
EvalInfo DisneyGlass::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	bool inside = info.frontFace;
	float etai_over_etat = inside ? (1.0f / eta) : (eta);
	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float roughness = roughnessTexture->Value(info.uv, info.position).r;
	float anisotropic = anisotropicTexture->Value(info.uv, info.position).r;

	float aspect = sqrt(1.0f - 0.9f * anisotropic);
	float ax = std::max(0.0001f, sqr(roughness) / aspect);
	float ay = std::max(0.0001f, sqr(roughness) * aspect);

	vec3 N = info.normal;
	vec3 H;
	float NdotV = abs(dot(N, V));
	float NdotL = abs(dot(N, L));

	vec3 bsdf;
	float pdf;
	float dwh_dwi;
	bool isReflect = dot(N, L) * dot(N, V) > 0.0f;
	if (isReflect) {
		H = normalize(V + L);
		float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);
		float G = GGX::GeometrySmith_1(V, H, N, ax, ay) * GGX::GeometrySmith_1(L, H, N, ax, ay);
		float D = GGX::DistributionGGX(H, N, ax, ay);

		bsdf = albedo * F * D * G / (4.0f * NdotL * NdotV);
		float Dv = GGX::DistributionVisibleGGX(V, H, N, ax, ay);
		dwh_dwi = abs(1.0f / (4.0f * dot(V, H)));
		pdf = Dv * dwh_dwi * (isReflect ? F : 1 - F);
	}
	else {
		H = -normalize(etai_over_etat * V + L);
		if (dot(N, H) < 0.0f) {
			H = -H;
		}

		float F = Fresnel::FresnelDielectric(V, H, etai_over_etat);
		float G = GGX::GeometrySmith_1(V, H, N, ax, ay) * GGX::GeometrySmith_1(L, H, N, ax, ay);
		float D = GGX::DistributionGGX(H, N, ax, ay);
		float HdotV = dot(H, V);
		float HdotL = dot(H, L);
		float sqrtDenom = etai_over_etat * HdotV + HdotL;
		float factor = abs(HdotL * HdotV / (NdotL * NdotV));

		float Dv = GGX::DistributionVisibleGGX(V, H, N, ax, ay);
		bsdf = sqrt(albedo) * (1.0f - F) * D * G * factor / sqr(sqrtDenom);
		dwh_dwi = abs(HdotL) / sqr(sqrtDenom);
		pdf = Dv * dwh_dwi * (isReflect ? F : 1 - F);
	}

	return { bsdf, NdotL, pdf };
}

vec3 DisneyGlass::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample DisneySheen::Sample(const vec3& V, const IntersectionInfo& info) {
	vec3 N = info.normal;

	vec2 sample(RandomFloat(), RandomFloat());

	vec3 L = CosWeight::Sample(info.normal, sample);

	float NdotL = dot(info.normal, L);
	if (NdotL < 0.0f) {
		return BsdfSampleError();
	}

	float pdf = NdotL * INV_PI;

	return { L, pdf };
}
EvalInfo DisneySheen::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 N = info.normal;
	vec3 H = normalize(V + L);
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float HdotL = dot(H, L);
	float HdotV = dot(H, V);

	if (NdotL <= 0.0f || NdotV <= 0.0f) {
		return { vec3(0.0f), NdotL, 1.0f };
	}

	float pdf = NdotL * INV_PI;

	vec3 albedo = albedoTexture->Value(info.uv, info.position);
	float sheenTint = sheenTintTexture->Value(info.uv, info.position).r;

	float clum = Luminance(albedo);
	vec3 ctint = clum > 0.0f ? albedo / clum : vec3(1.0f);
	vec3 csheen = glm::mix(vec3(1.0f), ctint, sheenTint);

	float FH = Fresnel::SchlickWeight(abs(HdotL));
	vec3 brdf = FH * csheen;

	return { brdf, NdotL, pdf };
}

vec3 DisneySheen::GetAlbedo(const IntersectionInfo& info) {
	return albedoTexture->Value(info.uv, info.position);
}

BsdfSample DisneyPrinciple::Sample(const vec3& V, const IntersectionInfo& info) {
	float metallic = metallicTexture->Value(info.uv, info.position).r;
	float specularTransmission = specularTransmissionTexture->Value(info.uv, info.position).r;
	vec3 sheen = sheenTexture->Value(info.uv, info.position);
	float clearcoat = clearcoatTexture->Value(info.uv, info.position).r;

	float diffuseWeight = (1.0f - metallic) * (1.0f - specularTransmission);
	float metalWeight = (1.0f - specularTransmission) * (1.0f - metallic);
	float glassWeight = (1.0f - metallic) * specularTransmission;
	float clearcoatWeight = 0.25f * clearcoat;

	float sum = diffuseWeight + metalWeight + glassWeight + clearcoatWeight;
	diffuseWeight /= sum;
	metalWeight /= sum;
	glassWeight /= sum;
	clearcoatWeight /= sum;

	float cdf_clearcoat = clearcoatWeight;
	float cdf_metal = cdf_clearcoat + metalWeight;
	float cdf_glass = cdf_metal + glassWeight;

	vec3 N = info.normal;

	BsdfSample bsdf_sample;
	if (!info.frontFace) {
		bsdf_sample = glass_m->Sample(V, info);

		return { bsdf_sample.bsdf_dir, bsdf_sample.bsdf_pdf };
	}

	float rnd = RandomFloat();
	if (rnd < cdf_clearcoat) {
		bsdf_sample = clearcoat_m->Sample(V, info);
	}
	else if (rnd < cdf_metal) {
		bsdf_sample = metal_m->Sample(V, info);
	}
	else if (rnd < cdf_glass) {
		bsdf_sample = glass_m->Sample(V, info);
	}
	else {
		bsdf_sample = diffuse_m->Sample(V, info);
	}

	vec3 L = bsdf_sample.bsdf_dir;
	EvalInfo diffuse_info = diffuse_m->Eval(V, L, info);
	EvalInfo metal_info = metal_m->Eval(V, L, info);
	EvalInfo clearcoat_info = clearcoat_m->Eval(V, L, info);
	EvalInfo glass_info = glass_m->Eval(V, L, info);

	float pdf = glassWeight * glass_info.bsdf_pdf;
	pdf += diffuseWeight * diffuse_info.bsdf_pdf;
	pdf += metalWeight * metal_info.bsdf_pdf;
	pdf += clearcoatWeight * clearcoat_info.bsdf_pdf;

	return { L, pdf };
}

EvalInfo DisneyPrinciple::Eval(const vec3& V, const vec3& L, const IntersectionInfo& info) {
	vec3 N = info.normal;

	float metallic = metallicTexture->Value(info.uv, info.position).r;
	float specularTransmission = specularTransmissionTexture->Value(info.uv, info.position).r;
	vec3 sheen = sheenTexture->Value(info.uv, info.position);
	float clearcoat = clearcoatTexture->Value(info.uv, info.position).r;

	float diffuseWeight = (1.0f - metallic) * (1.0f - specularTransmission);
	float metalWeight = (1.0f - specularTransmission) * (1.0f - metallic);
	float glassWeight = (1.0f - metallic) * specularTransmission;
	float clearcoatWeight = 0.25f * clearcoat;

	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	vec3 H;
	vec3 f_diffuse(0.0f);
	vec3 f_sheen(0.0f);
	vec3 f_metal(0.0f);
	vec3 f_clearcoat(0.0f);
	vec3 f_glass(0.0f);
	EvalInfo glass_info = glass_m->Eval(V, L, info);
	f_glass = glass_info.bsdf;
	float pdf = glass_info.bsdf_pdf;
	if (info.frontFace) {
		pdf *= glassWeight;

		EvalInfo diffuse_info = diffuse_m->Eval(V, L, info);
		EvalInfo sheen_info = sheen_m->Eval(V, L, info);
		EvalInfo metal_info = metal_m->Eval(V, L, info);
		EvalInfo clearcoat_info = clearcoat_m->Eval(V, L, info);

		f_diffuse = diffuse_info.bsdf;
		f_sheen = sheen_info.bsdf;
		f_metal = metal_info.bsdf;
		f_clearcoat = clearcoat_info.bsdf;

		pdf += diffuseWeight * diffuse_info.bsdf_pdf;
		pdf += metalWeight * metal_info.bsdf_pdf;
		pdf += clearcoatWeight * clearcoat_info.bsdf_pdf;
	}

	vec3 disney_bsdf = diffuseWeight * f_diffuse + (1.0f - metallic) * sheen * f_sheen + metalWeight * f_metal + clearcoatWeight * f_clearcoat + glassWeight * f_glass;
	
	return { disney_bsdf, NdotL, pdf };
}

vec3 DisneyPrinciple::GetAlbedo(const IntersectionInfo& info) {
	return diffuse_m->GetAlbedo(info);
}