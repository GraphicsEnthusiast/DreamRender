#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Transform
 * @brief Represents 3D affine transformations using homogeneous coordinates
 */
class Transform {
public:
    /**
     * @brief Default constructor (initializes identity transform)
     */
    Transform() : is_identity(true), transform_matrix(Matrix4f(1.0f)) {}

    /**
     * @brief Construct from explicit transformation matrix
     * @param m 4x4 transformation matrix (column-major)
     */
    explicit Transform(const Matrix4f& m) : is_identity(false), transform_matrix(m) {}

    /**
     * @brief Transforms a 3D point (applies translation)
     * @param p Input point in local space
     * @return Transformed point in world space
     */
    Point3f TransformPoint(const Point3f& p) const;

    /**
     * @brief Transforms a 3D vector (ignores translation)
     * @param v Input vector in local space
     * @return Transformed vector in world space
     */
    Vector3f TransformVector(const Vector3f& v) const;

    /**
     * @brief Retrieves underlying transformation matrix
     * @return 4x4 homogeneous transformation matrix
     */
    Matrix4f Matrix() const;

    /**
     * @brief Computes inverse transformation
     * @return New transform representing inverse operation
     */
    Transform Inverse() const;

    /**
     * @brief Concatenates transformations (A * B)
     * @param t Transform to concatenate
     * @return New combined transformation
     */
    Transform operator*(const Transform& t) const;

    /**
     * @brief Creates translation transformation
     * @param x Translation along X-axis
     * @param y Translation along Y-axis
     * @param z Translation along Z-axis
     */
    static Transform Translate(float x, float y, float z);

    /**
     * @brief Creates rotation transformation (Euler angles)
     * @param rx Rotation around X-axis (degrees)
     * @param ry Rotation around Y-axis (degrees)
     * @param rz Rotation around Z-axis (degrees)
     */
    static Transform Rotate(float rx, float ry, float rz);

    /**
     * @brief Creates scaling transformation
     * @param sx Scale factor along X-axis
     * @param sy Scale factor along Y-axis
     * @param sz Scale factor along Z-axis
     */
    static Transform Scale(float sx, float sy, float sz);

protected:
    bool is_identity;          ///< Identity transformation flag
    Matrix4f transform_matrix; ///< 4x4 homogeneous transformation matrix
};

NAMESPACE_END(dream)