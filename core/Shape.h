#pragma once

#include "Utils.h"
#include "Transform.h"
#include "Material.h"

enum ShapeType {
	TriangleMeshShape,
	SphereShape,
	QuadShape,
};

struct ShapeParams {
	ShapeType type;
	Transform transform;
	std::shared_ptr<Material> material;
	std::shared_ptr<Medium> out_medium;
	std::shared_ptr<Medium> in_medium;
	std::string file;
	Point3f center;
	float radius;
	Point3f position;
	Vector3f u;
	Vector3f v;
};

class Shape {
	friend Light;

public:
	Shape(ShapeType type, std::shared_ptr<Material> m, const Transform& trans, std::shared_ptr<Medium> out = NULL , std::shared_ptr<Medium> in = NULL, int geom_id = 0) :
	    m_type(type), material(m), transform(trans), out_medium(out), in_medium(in), geometry_id(geom_id) {}

	inline ShapeType GetType() const {
		return m_type;
	}

	inline int GetGeometryID() const {
		return geometry_id;
	}

	inline std::shared_ptr<Material> GetMaterial() const {
		return material;
	}

	inline std::shared_ptr<Medium> GetOutMedium() const {
		return out_medium;
	}

	inline std::shared_ptr<Medium> GetInMedium() const {
		return in_medium;
	}

	inline virtual Vector3f GetGeometryNormal(uint32_t faceID) const {
		return Point3f(0.0f);
	}

	inline virtual Vector3f GetShadeNormal(uint32_t faceID, const Point2f& barycentric) const { 
		return Vector3f(0.0f); 
	}

	inline virtual Point2f GetTexcoords(uint32_t faceID, const Point2f& barycentric) const { 
		return Point2f(0.0f); 
	}

	// Creating and committing the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) = 0;

	static Shape* Create(const ShapeParams& params);

protected:
	ShapeType m_type;
	Transform transform;
	int geometry_id;
	std::shared_ptr<Material> material;
	std::shared_ptr<Medium> out_medium;
	std::shared_ptr<Medium> in_medium;
};

class TriangleMesh : public Shape {
	friend TriangleMeshArea;

public:
	TriangleMesh(std::shared_ptr<Material> m, const std::string& file, const Transform& trans, std::shared_ptr<Medium> out = NULL, std::shared_ptr<Medium> in = NULL);

	inline uint32_t Vertices() const { 
		return vertices.size() / 3; 
	}

	inline uint32_t Faces() const { 
		return indices.size() / 3; 
	}

	inline Point3u GetIndices(uint32_t faceID) const {
		return Point3u(indices[3 * faceID + 0], indices[3 * faceID + 1], indices[3 * faceID + 2]);
	}

	// return vertex
	inline Vector3f GetVertex(uint32_t vertexID) const {
		return Vector3f(vertices[3 * vertexID + 0], vertices[3 * vertexID + 1],
			vertices[3 * vertexID + 2]);
	}

	// return vertex normal
	inline Vector3f GetVertexNormal(uint32_t vertexID) const {
		return Vector3f(normals[3 * vertexID + 0], normals[3 * vertexID + 1],
			normals[3 * vertexID + 2]);
	}

	// return vertex texcoords of specified face
	inline Point2f GetVertexTexcoords(uint32_t vertexID) const {
		return Point2f(texcoords[2 * vertexID + 0], texcoords[2 * vertexID + 1]);
	}

	// compute geometry normal
	inline virtual Vector3f GetGeometryNormal(uint32_t faceID) const override {
		const Point3u vidx = GetIndices(faceID);
		const Point3f A = GetVertex(vidx.x);
		const Point3f B = GetVertex(vidx.y);
		const Point3f C = GetVertex(vidx.z);

		return glm::normalize(glm::cross(B - A, C - A));
	}

	// compute shade normal of specified face, barycentric
	inline Vector3f GetShadeNormal(uint32_t faceID, const Point2f& barycentric) const override {
		const Point3u vidx = GetIndices(faceID);
		const Vector3f n1 = GetVertexNormal(vidx.x);
		const Vector3f n2 = GetVertexNormal(vidx.y);
		const Vector3f n3 = GetVertexNormal(vidx.z);

		return glm::normalize(n1 * (1.0f - barycentric[0] - barycentric[1]) + n2 * barycentric[0] +
			n3 * barycentric[1]);
	}

	// compute texcoords of specified face, barycentric
	inline Point2f GetTexcoords(uint32_t faceID, const Point2f& barycentric) const override {
		const Point3u vidx = GetIndices(faceID);
		const Point2f t1 = GetVertexTexcoords(vidx.x);
		const Point2f t2 = GetVertexTexcoords(vidx.y);
		const Point2f t3 = GetVertexTexcoords(vidx.z);

		return t1 * (1.0f - barycentric[0] - barycentric[1]) + t2 * barycentric[0] +
			t3 * barycentric[1];
	}

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;

private:
	std::vector<float> vertices;
	std::vector<uint32_t> indices;
	std::vector<float> normals;
	std::vector<float> texcoords;
};

class Sphere : public Shape {
	friend SphereArea;

public:
	Sphere(std::shared_ptr<Material> m, Point3f cen, float rad, std::shared_ptr<Medium> out = NULL, std::shared_ptr<Medium> in = NULL) : 
		Shape(ShapeType::SphereShape, m, Transform(), out, in), center(cen), radius(rad) {}

	// User defined intersection functions for the Sphere primitive
	static void SphereBoundsFunc(const struct RTCBoundsFunctionArguments* args);

	static void SphereIntersectFunc(const RTCIntersectFunctionNArguments* args);

	static void SphereOccludedFunc(const RTCOccludedFunctionNArguments* args);

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;

	static Point2f GetSphereUV(const Point3f& surface_pos, const Point3f& center);

private:
	Point3f center;
	float radius;
};

class Quad : public Shape {
	friend QuadArea;

public:
	Quad(std::shared_ptr<Material> m, const Point3f& pos, const Vector3f& uu, const Vector3f& vv, std::shared_ptr<Medium> out = NULL, std::shared_ptr<Medium> in = NULL) :
		Shape(ShapeType::QuadShape, m, Transform(), out, in), position(pos), u(uu), v(vv) {}

	// Creating and commiting the current object to Embree scene
	virtual int ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) override;

	static Point2f GetQuadUV(const Point3f& p, const Point3f& position, const Vector3f& u, const Vector3f& v);

private:
	Point3f position;
	Vector3f u;
	Vector3f v;
};