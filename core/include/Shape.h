#pragma once

#include <Utils.h>
#include <Material.h>

enum class ShapeType {
	TriangleMesh,
	Sphere,
	Quad,
};

class Shape {
public:
	Shape(shared_ptr<Material> mat, vec3 pos, mat4 trans, int geom_id = 0);

	inline virtual vec3 GetFaceNormal(uint32_t faceID, const vec2& barycentric) const { return vec3(0.0f); }
	inline virtual vec2 GetTexcoords(uint32_t faceID, const vec2& barycentric) const { return vec2(0.0f); }

	// Creating and committing the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) = 0;
	virtual float Pdf(const IntersectionInfo& info, const vec3& L, float dist) { return 0.0f; }

public:
	// Position before any transformation
	ShapeType m_type;
	vec3 position;
	mat4 transform;
	int geometry_id;
	shared_ptr<Material> material;
};

class TriangleMesh : public Shape {
public:
	TriangleMesh(shared_ptr<Material> mat, string file, mat4 trans, vec3 pos = vec3(0.0f));
	~TriangleMesh();

	inline uint32_t Vertices() const { return vertices.size() / 3; }
	inline uint32_t Faces() const { return indices.size() / 3; }

	struct VertexIndices {
		uint32_t v1idx;
		uint32_t v2idx;
		uint32_t v3idx;
	};

	VertexIndices GetIndices(uint32_t faceID) const {
		VertexIndices ret;
		ret.v1idx = indices[3 * faceID + 0];
		ret.v2idx = indices[3 * faceID + 1];
		ret.v3idx = indices[3 * faceID + 2];
		return ret;
	}

	// return vertex normal
	inline vec3 GetVertexNormal(uint32_t vertexID) const {
		return vec3(normals[3 * vertexID + 0], normals[3 * vertexID + 1],
			normals[3 * vertexID + 2]);
	}

	// return vertex texcoords
	inline vec2 GetVertexTexcoords(uint32_t vertexID) const {
		return vec2(texcoords[2 * vertexID + 0], texcoords[2 * vertexID + 1]);
	}

	// compute normal of specified face, barycentric
	inline vec3 GetFaceNormal(uint32_t faceID, const vec2& barycentric) const override {
		const VertexIndices vidx = GetIndices(faceID);
		const vec3 n1 = GetVertexNormal(vidx.v1idx);
		const vec3 n2 = GetVertexNormal(vidx.v2idx);
		const vec3 n3 = GetVertexNormal(vidx.v3idx);

		return n1 * (1.0f - barycentric[0] - barycentric[1]) + n2 * barycentric[0] +
			n3 * barycentric[1];
	}

	// compute texcoords of specified face, barycentric
	inline vec2 GetTexcoords(uint32_t faceID, const vec2& barycentric) const override {
		const VertexIndices vidx = GetIndices(faceID);
		const vec2 t1 = GetVertexTexcoords(vidx.v1idx);
		const vec2 t2 = GetVertexTexcoords(vidx.v2idx);
		const vec2 t3 = GetVertexTexcoords(vidx.v3idx);

		return t1 * (1.0f - barycentric[0] - barycentric[1]) + t2 * barycentric[0] +
			t3 * barycentric[1];
	}

	int LoadFromObj(RTCDevice& rtc_device, RTCScene& rtc_scene);

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;

public:
	string filename;
	std::vector<float> vertices;
	std::vector<uint32_t> indices;
	std::vector<float> normals;
	std::vector<float> texcoords;
};

class Sphere : public Shape {
public:
	Sphere(shared_ptr<Material> mat, vec3 cen, float rad, vec3 pos = vec3(0.0f), mat4 trans = mat4(1.0f));

	// User defined intersection functions for the Sphere primitive
	static void SphereBoundsFunc(const struct RTCBoundsFunctionArguments* args);
	static void SphereIntersectFunc(const RTCIntersectFunctionNArguments* args);
	static void SphereOccludedFunc(const RTCOccludedFunctionNArguments* args);
	static vec2 GetSphereUV(const vec3& p, const vec3& center);

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;
	virtual float Pdf(const IntersectionInfo& info, const vec3& L, float dist) override;

public:
	float radius;
	vec3 center;
};

class Quad : public Shape {
public:
	Quad(shared_ptr<Material> mat, vec3 p, vec3 _u, vec3 _v, vec3 pos = vec3(0.0f), mat4 trans = mat4(1.0f));

	static vec2 GetQuadUV(const vec3& p, const Quad* quad);

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;
	virtual float Pdf(const IntersectionInfo& info, const vec3& L, float dist) override;

public:
	vec3 position;
	vec3 u;
	vec3 v;
};