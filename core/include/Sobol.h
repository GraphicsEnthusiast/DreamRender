#pragma once

#include <Utils.h>
#include <SobolMatrices1024x52.h>

inline uint GrayCode(uint i) {
	return i ^ (i >> 1);
}

inline float Sobol(uint d, uint i) {
	uint result = 0;
	uint offset = d * SobolMatricesSize;
	for (uint j = 0; i != 0; i >>= 1, j++) {
		if ((i & 1) != 0) {
			result ^= SobolMatrices[j + offset];
		}
	}

	return float(result) * (1.0f / float(0xFFFFFFFFU));
}

inline vec2 SobolVec2(uint i, uint b) {
	float u = Sobol(b * 2, GrayCode(i));
	float v = Sobol(b * 2 + 1, GrayCode(i));

	return vec2(u, v);
}

inline uint WangHash(uint seed) {
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);

	return seed;
}

inline vec2 CranleyPattersonRotation(vec2 p, vec2 pix) {
	uint pseed = uint(
		uint(pix.x) * uint(1973) +
		uint(pix.y) * uint(9277) +
		uint(114514 / 1919) * uint(26699)) | uint(1);

	float u = float(WangHash(pseed)) / 4294967296.0f;
	float v = float(WangHash(pseed)) / 4294967296.0f;

	p.x += u;
	if (p.x > 1.0f) {
		p.x -= 1.0f;
	}
	if (p.x < 0) {
		p.x += 1.0f;
	}

	p.y += v;
	if (p.y > 1) {
		p.y -= 1.0f;
	}
	if (p.y < 0) {
		p.y += 1.0f;
	}

	return p;
}
