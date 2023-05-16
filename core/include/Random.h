#pragma once

#include <Utils.h>

using namespace glm;

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

//0-1, 随机数
inline float RandomFloat() {
	return static_cast<float>(rand() / (RAND_MAX + 1.0f));
}

//min-max, 随机数
inline float RandomFloat(float min, float max) {
	return min + (max - min) * RandomFloat();
}

inline vec3 Random() {
	return vec3(RandomFloat(), RandomFloat(), RandomFloat());
}

inline vec3 Random(float min, float max) {
	return vec3(RandomFloat(min, max), RandomFloat(min, max), RandomFloat(min, max));
}

// inline vec3 RandomInUnitSphere() {
// 	while (true) {
// 		vec3 p = Random(-1.0f, 1.0f);
// 		if (length(p) * length(p) >= 1.0f) {
// 			continue;
// 		}
// 		return p;
// 	}
// }

//半球均匀采样
// inline vec3 SampleHemisphere(float xi_1, float xi_2) {
// 	//xi_1 = rand(), xi_2 = rand();
// 	float z = xi_1;
// 	float r = std::max(0.0f, sqrt(1.0f - z * z));
// 	float phi = 2.0f * PI * xi_2;
// 	vec3 L(r * cos(phi), r * sin(phi), z);
// 
// 	return L;
// }