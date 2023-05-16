#pragma once

#include <Utils.h>

class Filter {
public:
	virtual vec2 FilterVec2(const vec2& r2) = 0;
};

class FilterBox : public Filter {
public:
	FilterBox() = default;

	inline virtual vec2 FilterVec2(const vec2& r2) override {
		return r2 - vec2(0.5f, 0.5f);
	}
};

class FilterTent : public Filter {
public:
	FilterTent() = default;

	inline virtual vec2 FilterVec2(const vec2& r2) override {
		vec2 j = r2;
		j = j * 2.0f;
		j.x = j.x < 1.0f ? sqrt(j.x) - 1.0f : 1.0f - sqrt(2.0f - j.x);
		j.y = j.y < 1.0f ? sqrt(j.y) - 1.0f : 1.0f - sqrt(2.0f - j.y);

		return vec2(0.5f, 0.5f) + j;
	}
};

class FilterTriangle : public Filter {
public:
	FilterTriangle() = default;

	inline virtual vec2 FilterVec2(const vec2& r2) override {
		float u1 = r2.x;
		float u2 = r2.y;
		if (u2 > u1) {
			u1 *= 0.5f;
			u2 -= u1;
		}
		else {
			u2 *= 0.5f;
			u1 -= u2;
		}

		return vec2(0.5f, 0.5f) + vec2(u1, u2);
	}
};

class FilterGaussian : public Filter {
public:
	FilterGaussian() = default;

	inline virtual vec2 FilterVec2(const vec2& r2) override {
		float r1 = std::max(FLT_MIN, r2.x);
		float r = sqrt(-2.0f * log(r1));
		float theta = 2.0f * PI * r2.y;
		vec2 uv = r * vec2(cos(theta), sin(theta));

		return vec2(0.5f, 0.5f) + 0.375f * uv;
	}
};