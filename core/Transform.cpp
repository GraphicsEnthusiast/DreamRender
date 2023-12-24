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

Transform Transform::Perspective(float fov, float nearClip, float farClip) {
	float invTan = 1.0f / std::tan(glm::radians(fov * 0.5f));
	Matrix4f perMat;

	perMat[0][0] = invTan;
	perMat[1][0] = 0.0f;
	perMat[2][0] = 0.0f;
	perMat[3][0] = 0.0f;

	perMat[0][1] = 0.0f;
	perMat[1][1] = invTan;
	perMat[2][1] = 0.0f;
	perMat[3][1] = 0.0f;

	perMat[0][2] = 0.0f;
	perMat[1][2] = 0.0f;
	perMat[2][2] = farClip / (farClip - nearClip);
	perMat[3][2] = -farClip * nearClip / (farClip - nearClip);

	perMat[0][3] = 0.0f;
	perMat[1][3] = 0.0f;
	perMat[2][3] = 1.0f;
	perMat[3][3] = 0.0f;

	return Transform(perMat);
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

Transform Transform::LookAt(const Point3f& origin, const Point3f& target, const Vector3f& up) {
	Matrix4f mat;
	mat[3][0] = origin[0];
	mat[3][1] = origin[1];
	mat[3][2] = origin[2];
	mat[3][3] = 1.0f;

	Vector3f forward = glm::normalize(target - origin);
	Vector3f left = glm::cross(up, forward);
	Vector3f realUp = glm::cross(forward, left);
	mat[0][0] = left[0];
	mat[0][1] = left[1];
	mat[0][2] = left[2];
	mat[0][3] = 0.0f;

	mat[1][0] = realUp[0];
	mat[1][1] = realUp[1];
	mat[1][2] = realUp[2];
	mat[1][3] = 0.0f;

	mat[2][0] = forward[0];
	mat[2][1] = forward[1];
	mat[2][2] = forward[2];
	mat[2][3] = 0.0f;

	return Transform(mat);
}