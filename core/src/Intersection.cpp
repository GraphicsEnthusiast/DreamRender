#include <Intersection.h>

void IntersectionInfo::SetFaceNormal(const vec3& dir, const vec3& outward_normal) {
	frontFace = dot(dir, outward_normal) < 0.0f;
	normal = frontFace ? outward_normal : -outward_normal;
}