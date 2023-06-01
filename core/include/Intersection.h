#pragma once

#include <Utils.h>

class Material;
class Medium;
class IntersectionInfo {
public:
	IntersectionInfo() = default;

	void SetFaceNormal(const vec3& dir, const vec3& outward_normal);

public:
	float t;//从光线起点到交互点的距离
	vec2 uv;//交互点纹理坐标
	vec3 position;//交互点空间坐标
	vec3 normal;//交互点法线
	bool frontFace;//景物表面的交互点是否位于景物内部
	vec2 pixel_ndc;//当前像素的ndc坐标
};