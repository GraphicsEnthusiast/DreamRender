#include "Transform.h"

Point3f Transform::TransformPoint(const Point3f& p) const {
	if (identity) {
		return p;
	}

	Point4f ret = transformMatrix * Vector4f(p, 1.0f);
	assert(ret[3] != 0.0f);
	if (ret[3] == 1.0f) {
		return Vector3f(ret[0], ret[1], ret[2]);
	}
	else {
		return Vector3f(ret[0], ret[1], ret[2]) / ret[3];
	}
}

Vector3f Transform::TransformVector(const Vector3f& v) const {
	if (identity) {
		return v;
	}

	return transformMatrix * Vector4f(v, 0.0f);
}

Matrix4f Transform::Mat() const {
	return transformMatrix;
}

Transform Transform::Inverse() const {
	if (identity) {
		return Transform();
	}

	return Transform(glm::inverse(transformMatrix));
}

Transform Transform::operator*(const Transform& t) const {
	return Transform(transformMatrix * t.transformMatrix);
}

Transform Transform::Translate(float x, float y, float z) {
	Vector3f offset(x, y, z);
	Matrix4f m(1.0f);
	m = glm::translate(m, offset);

	return Transform(m);
}

Transform Transform::Rotate(float rx, float ry, float rz) {
	Matrix4f m(1.0f);
	m = glm::rotate(m, glm::radians(rx), Vector3f(1.0f, 0.0f, 0.0f));
	m = glm::rotate(m, glm::radians(ry), Vector3f(0.0f, 1.0f, 0.0f));
	m = glm::rotate(m, glm::radians(rz), Vector3f(0.0f, 0.0f, 1.0f));

	return Transform(m);
}

Transform Transform::Scale(float sx, float sy, float sz) {
	Vector3f factor(sx, sy, sz);
	Matrix4f m(1.0f);
	m = glm::scale(m, factor);

	return Transform(m);
}