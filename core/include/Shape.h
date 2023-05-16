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

	int LoadFromObj(RTCDevice& rtc_device, RTCScene& rtc_scene);

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;

public:
	string filename;
	vector<vec3> vertices;
	vector<vec3> normals;
	vector<vec2> uvs;
	vector<int> indices_v;
	vector<int> indices_n;
	vector<int> indices_t;
	vec3* aligned_normals;
	vec2* aligned_uvs;
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

class Quad final : public Shape {
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