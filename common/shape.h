#pragma once

#include <transform.h>

NAMESPACE_BEGIN(dream)

class TriangleMesh {
public:
	TriangleMesh(const std::string& file, const Transform& trans);

protected:
	Transform transform;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	std::vector<float> normals;
	std::vector<float> texcoords;
};

NAMESPACE_END(dream)