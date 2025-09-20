#ifndef _FILTER_SAMPLER_GLSL_
#define _FILTER_SAMPLER_GLSL_

#include "util/util.glsl"

vec2 GaussianFilter(vec2 sample_xy) {
	float r1 = max(1e-6f, sample_xy.x);
	float r = sqrt(-2.0f * log(r1));
	float theta = 2.0f * PI * sample_xy.y;
	vec2 uv = r * vec2(cos(theta), sin(theta));

	return vec2(0.5f) + 0.375f * uv;
}

#endif // _FILTER_SAMPLER_GLSL_