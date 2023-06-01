#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <memory>
#include <math.h>
#include <array>
#include <queue>
#include <omp.h>
#include <map>
#include <cmath>
#include <numeric>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/optimum_pow.hpp>
#include <glm/gtx/string_cast.hpp>
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

using namespace std;
using namespace glm;

static const float OneMinusEpsilon = 0x1.fffffep-1f;
static const float INF = numeric_limits<float>::infinity();
static const float EPS = 1e-4f;
static const float PI = 3.14159265358979323846f;
static const float INV_PI = 0.31830988618379067154f;
static const float INV_2PI = 0.15915494309189533577f;
static const float INV_4PI = 0.07957747154594766788f;

inline float sqr(float x) {
	return x * x;
}

inline bool IsNan(const vec3& v) {
	return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

inline vec3 GetNormal(const RTCRayHit& rayhit) {
	return vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
}

inline vec3 GetRayOrg(const RTCRay& ray) {
	return vec3(ray.org_x, ray.org_y, ray.org_z);
}

inline vec3 GetRayOrg(const RTCRayHit& rayhit) {
	return GetRayOrg(rayhit.ray);
}

inline vec3 GetRayDir(const RTCRay& ray) {
	return vec3(ray.dir_x, ray.dir_y, ray.dir_z);
}

inline vec3 GetRayDir(const RTCRayHit& rayhit) {
	return GetRayDir(rayhit.ray);
}

inline vec3 GetHitPos(const RTCRay& ray) {
	auto org = GetRayOrg(ray);
	auto dir = GetRayDir(ray);

	return org + dir * ray.tfar;
}

inline vec3 GetHitPos(const RTCRayHit& rayhit) {
	auto org = GetRayOrg(rayhit);
	auto dir = GetRayDir(rayhit);

	return org + dir * rayhit.ray.tfar;
}

inline RTCRay MakeRay(const vec3& rayorg, const vec3& raydir, float tnear = EPS, float tfar = INF) {
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

inline RTCRayHit MakeRayHit(const vec3& rayorg, const vec3& raydir, float tnear = EPS, float tfar = INF) {
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

inline void SetRayOrg(RTCRayHit& rayhit, const vec3& off) {
	rayhit.ray.org_x = off.x;
	rayhit.ray.org_y = off.y;
	rayhit.ray.org_z = off.z;
}

// inline constexpr float Origin() { return 1.0f / 32.0f; }
// inline constexpr float FloatScale() { return 1.0f / 65536.0f; }
// inline constexpr float IntScale() { return 256.0f; }

// Normal points outward for rays exiting the surface, else is flipped.
// inline vec3 OffsetRay(const vec3& p, const vec3& n) {
// 	ivec3 of_i(IntScale() * n.x, IntScale() * n.y, IntScale() * n.z);
// 
// 	vec3 p_i(
// 		float(int(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
// 		float(int(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
// 		float(int(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));
// 
// 	return vec3(fabsf(p.x) < Origin() ? p.x + FloatScale() * n.x : p_i.x,
// 		fabsf(p.y) < Origin() ? p.y + FloatScale() * n.y : p_i.y,
// 		fabsf(p.z) < Origin() ? p.z + FloatScale() * n.z : p_i.z);
// }

inline mat4 GetTransformMatrix(vec3 translateCtrl, vec3 rotateCtrl, vec3 scaleCtrl) {
	mat4 unit(1.0f);
	mat4 scale_mat = scale(unit, scaleCtrl);
	mat4 translate_mat = translate(unit, translateCtrl);
	mat4 rotate_mat = unit;
	rotate_mat = rotate(rotate_mat, radians(rotateCtrl.x), vec3(1.0f, 0.0f, 0.0f));
	rotate_mat = rotate(rotate_mat, radians(rotateCtrl.y), vec3(0.0f, 1.0f, 0.0f));
	rotate_mat = rotate(rotate_mat, radians(rotateCtrl.z), vec3(0.0f, 0.0f, 1.0f));

	mat4 model = translate_mat * rotate_mat * scale_mat;

	return model;
}

inline float Luminance(const vec3& c) {
	return dot(vec3(0.299f, 0.587f, 0.114f), c);
}

//判断两个向量的方向是否相同，用于平滑塑料，在一定误差范围内相同就行
inline bool SameDirection(const vec3& a, const vec3& b) {
	return abs(dot(a, b) - 1.0f) < 0.1f;
}

//将单位向量从世界坐标系转换到局部坐标系
//dir 待转换的单位向量
//up 局部坐标系的竖直向上方向在世界坐标系下的方向
inline vec3 ToLocal(const vec3& dir, const vec3& up) {
	auto B = vec3(0.0f), C = vec3(0.0f);
	if (abs(up.x) > abs(up.y)) {
		float len_inv = 1.0f / sqrt(up.x * up.x + up.z * up.z);
		C = vec3(up.z * len_inv, 0.0f, -up.x * len_inv);
	}
	else {
		float len_inv = 1.0f / sqrt(up.y * up.y + up.z * up.z);
		C = vec3(0.0f, up.z * len_inv, -up.y * len_inv);
	}
	B = cross(C, up);

	return vec3(dot(dir, B), dot(dir, C), dot(dir, up));
}

//将单位向量从局部坐标系转换到世界坐标系
//dir 待转换的单位向量
//up 局部坐标系的竖直向上方向在世界坐标系下的方向
inline vec3 ToWorld(const vec3& dir, const vec3& up) {
	auto B = vec3(0.0f), C = vec3(0.0f);
	if (abs(up.x) > abs(up.y)) {
		float len_inv = 1.0f / sqrt(up.x * up.x + up.z * up.z);
		C = vec3(up.z * len_inv, 0.0f, -up.x * len_inv);
	}
	else {
		float len_inv = 1.0f / sqrt(up.y * up.y + up.z * up.z);
		C = vec3(0.0f, up.z * len_inv, -up.y * len_inv);
	}
	B = cross(C, up);

	return normalize(dir.x * B + dir.y * C + dir.z * up);
}