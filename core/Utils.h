#pragma once

#include <iostream>
#include <iomanip>
#include <omp.h>
#include <assert.h>
#include <string>
#include <algorithm>
#include <cinttypes>
#include <random>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <queue>
#include <vector>
#include <time.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

template <int nSpectrumSamples>
class CoefficientSpectrum;
class RGBSpectrum;// Now only use rgb
class SampledSpectrum;

class ToneMapper;
class Reinhard;
class Uncharted2;
class ACES;

class Transform;

class Shape;
class TriangleMesh;
class Sphere;
class Quad;

class Camera;
class Pinhole;
class Thinlens;

class Ray;

class AliasTable1D;
class AliasTable2D;

class Sampler;
class Independent;
class SimpleSobol;

class Texture;
class Constant;
class Image;
class Hdr;

using Vector2u = glm::uvec2;
using Vector2i = glm::ivec2;
using Vector2f = glm::vec2;
using Vector3u = glm::uvec3;
using Vector3i = glm::ivec3;
using Vector3f = glm::vec3;
using Vector4u = glm::uvec4;
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
using Point2u = Vector2u;
using Point3u = Vector3u;
using Point4u = Vector4u;

constexpr float FloatOneMinusEpsilon = 0x1.fffffep-1f;
constexpr float MaxFloat = std::numeric_limits<float>::max();
constexpr float Infinity = std::numeric_limits<float>::infinity();
constexpr float Epsilon = std::numeric_limits<float>::epsilon() * 0.5f;
constexpr float ShadowEpsilon = 1e-3f;
constexpr float PI = 3.1415926535897932385f;
constexpr float INV_PI = 1.0f / PI;
constexpr float INV_2PI = 1.0f / (2.0f * PI);
constexpr float INV_4PI = 1.0f / (4.0f * PI);

struct IntersectionInfo {
	float t;
	Point2f uv;
	Point3f position;
	Vector3f Ng;
	Vector3f Ns;
	bool frontFace;

	inline void SetNormal(const Vector3f& L, const Vector3f& _Ng, const Vector3f& _Ns) {
		frontFace = glm::dot(L, Ng) > 0.0f;
		Ng = frontFace ? _Ng : -_Ng;
		Ns = frontFace ? _Ns : -_Ns;
		if (glm::dot(Ng, Ns) < 0.0f) {
			Ns = glm::reflect(Ns, Ng);
		}
	}
};

inline Vector3f ToLocal(const Vector3f& up, const Vector3f& dir) {
	auto B = Vector3f(0.0f), C = Vector3f(0.0f);
	if (std::abs(up.x) > std::abs(up.y)) {
		float len_inv = 1.0f / std::sqrt(up.x * up.x + up.z * up.z);
		C = Vector3f(up.z * len_inv, 0.0f, -up.x * len_inv);
	}
	else {
		float len_inv = 1.0f / std::sqrt(up.y * up.y + up.z * up.z);
		C = Vector3f(0.0f, up.z * len_inv, -up.y * len_inv);
	}
	B = glm::cross(C, up);

	return Vector3f(glm::dot(dir, B), glm::dot(dir, C), glm::dot(dir, up));
}

inline Vector3f ToWorld(const Vector3f& up, const Vector3f& dir) {
	auto B = Vector3f(0.0f), C = Vector3f(0.0f);
	if (std::abs(up.x) > std::abs(up.y)) {
		float len_inv = 1.0f / std::sqrt(up.x * up.x + up.z * up.z);
		C = Vector3f(up.z * len_inv, 0.0f, -up.x * len_inv);
	}
	else {
		float len_inv = 1.0f / std::sqrt(up.y * up.y + up.z * up.z);
		C = Vector3f(0.0f, up.z * len_inv, -up.y * len_inv);
	}
	B = glm::cross(C, up);

	return glm::normalize(dir.x * B + dir.y * C + dir.z * up);
}

inline Vector3f GetNormal(const RTCRayHit& rayhit) {
	return Vector3f(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
}

inline Point3f GetRayOrg(const RTCRay& ray) {
	return Point3f(ray.org_x, ray.org_y, ray.org_z);
}

inline Point3f GetRayOrg(const RTCRayHit& rayhit) {
	return GetRayOrg(rayhit.ray);
}

inline Vector3f GetRayDir(const RTCRay& ray) {
	return Vector3f(ray.dir_x, ray.dir_y, ray.dir_z);
}

inline Vector3f GetRayDir(const RTCRayHit& rayhit) {
	return GetRayDir(rayhit.ray);
}

inline Point3f GetHitPos(const RTCRay& ray) {
	Point3f org = GetRayOrg(ray);
	Vector3f dir = GetRayDir(ray);

	return org + dir * ray.tfar;
}

inline Point3f GetHitPos(const RTCRayHit& rayhit) {
	Point3f org = GetRayOrg(rayhit);
	Vector3f dir = GetRayDir(rayhit);

	return org + dir * rayhit.ray.tfar;
}

inline RTCRay MakeRay(const Point3f& rayorg, const Vector3f& raydir, float tnear = 0.0f, float tfar = Infinity) {
	RTCRay ray;

	ray.org_x = rayorg.x;
	ray.org_y = rayorg.y;
	ray.org_z = rayorg.z;
	ray.dir_x = raydir.x;
	ray.dir_y = raydir.y;
	ray.dir_z = raydir.z;
	ray.tnear = tnear;
	ray.tfar = tfar;

	return ray;
}

inline RTCRayHit MakeRayHit(const Point3f& rayorg, const Vector3f& raydir, float tnear = 0.0f, float tfar = Infinity) {
	RTCRayHit rayhit;

	rayhit.ray.org_x = rayorg.x;
	rayhit.ray.org_y = rayorg.y;
	rayhit.ray.org_z = rayorg.z;
	rayhit.ray.dir_x = raydir.x;
	rayhit.ray.dir_y = raydir.y;
	rayhit.ray.dir_z = raydir.z;
	rayhit.ray.tnear = tnear;
	rayhit.ray.tfar = tfar;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

	return rayhit;
}

inline RTCRayHit MakeRayHit(const RTCRay& ray) {
	RTCRayHit rayhit;

	rayhit.ray = ray;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

	return rayhit;
}

inline void SetRayOrg(RTCRayHit& rayhit, const Point3f& org) {
	rayhit.ray.org_x = org.x;
	rayhit.ray.org_y = org.y;
	rayhit.ray.org_z = org.z;
}