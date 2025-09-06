#include <transform.h>>

NAMESPACE_BEGIN(dream)

Point3f Transform::TransformPoint(const Point3f& p) const {
	// Early return for identity transform
	if (is_identity) {
		return p;
	}

	// Apply homogeneous transformation: [x, y, z, 1]
	const Vector4f homog_point(p.x, p.y, p.z, 1.0f);
	const Vector4f transformed = transform_matrix * homog_point;

	// Handle perspective division if w �� 1
	if (1.0f == transformed.w) {
		return Point3f(transformed.x, transformed.y, transformed.z);
	}
	return Point3f(transformed.x, transformed.y, transformed.z) / transformed.w;
}

Vector3f Transform::TransformVector(const Vector3f& v) const {
	// Early return for identity transform
	if (is_identity) {
		return v;
	}

	// Apply vector transformation: [x, y, z, 0]
	const Vector4f homog_vector(v.x, v.y, v.z, 0.0f);
	const Vector4f transformed = transform_matrix * homog_vector;

	return Vector3f(transformed.x, transformed.y, transformed.z);
}

Matrix4f Transform::Matrix() const {
	return transform_matrix;
}

Transform Transform::Inverse() const {
	// Identity inverse is identity
	if (is_identity) {
		return Transform();
	}

	// Compute matrix inverse using GLM
	return Transform(glm::inverse(transform_matrix));
}

Transform Transform::operator*(const Transform& t) const {
	// Concatenate transformation matrices
	return Transform(transform_matrix * t.transform_matrix);
}

Transform Transform::Translate(float x, float y, float z) {
	Matrix4f m(1.0f);  // Start with identity

	return Transform(glm::translate(m, Vector3f(x, y, z)));
}

Transform Transform::Rotate(float rx, float ry, float rz) {
	Matrix4f m(1.0f);  // Start with identity

	// Apply rotations in ZYX order (Tait-Bryan angles)
	m = glm::rotate(m, glm::radians(rz), Vector3f(0.0f, 0.0f, 1.0f));
	m = glm::rotate(m, glm::radians(ry), Vector3f(0.0f, 1.0f, 0.0f));
	m = glm::rotate(m, glm::radians(rx), Vector3f(1.0f, 0.0f, 0.0f));

	return Transform(m);
}

Transform Transform::Scale(float sx, float sy, float sz) {
	Matrix4f m(1.0f);  // Start with identity

	return Transform(glm::scale(m, Vector3f(sx, sy, sz)));
}

NAMESPACE_END(dream)