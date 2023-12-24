#pragma once

#include "Utils.h"

class Transform {
public:
	Transform() : identity(true), transformMatrix(Matrix4f(1.0f)) {}

	Transform(const Matrix4f& transformMatrix) : identity(false), transformMatrix(transformMatrix) {}

	Point3f TransformPoint(const Point3f& p) const;

	Vector3f TransformVector(const Vector3f& v) const;

	Matrix4f Mat() const;

	Transform Inverse() const;

	Transform operator*(const Transform& t) const;

	static Transform Perspective(float fov, float nearClip, float farClip);

	static Transform Translate(float x, float y, float z);

	static Transform Rotate(float rx, float ry, float rz);

	static Transform Scale(float sx, float sy, float sz);

	static Transform LookAt(const Point3f& origin, const Point3f& target, const Vector3f& up);

private:
	bool identity;
	Matrix4f transformMatrix;
};