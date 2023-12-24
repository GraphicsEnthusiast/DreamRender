#pragma once

#include <iostream>
#include <iomanip>
#include <omp.h>
#include <assert.h>
#include <string>
#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <time.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

template <int nSpectrumSamples>
class CoefficientSpectrum;
class RGBSpectrum;// Now only use rgb
class SampledSpectrum;

class ToneMapper;
class Reinhard;
class Uncharted2;
class ACES;

using Vector2i = glm::ivec2;
using Vector2f = glm::vec2;
using Vector3i = glm::ivec3;
using Vector3f = glm::vec3;
using Vector4i = glm::ivec4;
using Vector4f = glm::vec4;
using Matrix3f = glm::mat3x3;
using Matrix4f = glm::mat4x4;

using Point2f = Vector2f;
using Point3f = Vector3f;
using Point4f = Vector4f;
using Point2i = Vector2i;
using Point3i = Vector3i;
using Point4i = Vector4i;

constexpr float FloatOneMinusEpsilon = 0x1.fffffep-1f;
constexpr float MaxFloat = std::numeric_limits<float>::max();
constexpr float Infinity = std::numeric_limits<float>::infinity();
constexpr float Epsilon = std::numeric_limits<float>::epsilon() * 0.5f;
constexpr float ShadowEpsilon = 0.0001f;
constexpr float PI = 3.1415926535897932385f;
constexpr float INV_PI = 1.0f / PI;
constexpr float INV_2PI = 1.0f / (2.0f * PI);
constexpr float INV_4PI = 1.0f / (4.0f * PI);

template <typename Predicate>
int FindInterval(int size, const Predicate& pred) {
	int first = 0, len = size;
	while (len > 0) {
		int half = len >> 1, middle = first + half;
		// Bisect range based on value of _pred_ at _middle_
		if (pred(middle)) {
			first = middle + 1;
			len -= half + 1;
		}
		else {
			len = half;
		}
	}

	return glm::clamp(first - 1, 0, size - 2);
}