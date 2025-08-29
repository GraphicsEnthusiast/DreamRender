#pragma once

#include <buffer_object.h>
#include <transform.h>

NAMESPACE_BEGIN(dream)

/**
 * @class TriangleMesh
 * @brief Represents a 3D triangle mesh with vertex attributes and buffer objects for GPU access
 */
class TriangleMesh {
public:
    /**
     * @brief Constructs a triangle mesh from a file with optional transformation
     * @param file Path to the mesh file to load
     * @param trans Transformation to apply to the mesh vertices during loading[1,4](@ref)
     */
    TriangleMesh(const std::string& file, const Transform& trans);

    /**
     * @brief Gets the vertex buffer object for compute shader access
     * @return Const reference to the vertex texture buffer object (TBO)[1,4](@ref)
     */
    const TBO& GetVertexTBO() const noexcept;

    /**
     * @brief Gets the normal buffer object for compute shader access
     * @return Const reference to the normal texture buffer object (TBO)[1,4](@ref)
     */
    const TBO& GetNormalTBO() const noexcept;

    /**
     * @brief Gets the texture coordinate buffer object for compute shader access
     * @return Const reference to the texture coordinate texture buffer object (TBO)[1,4](@ref)
     */
    const TBO& GetTexCoordTBO() const noexcept;

    /**
     * @brief Gets the index buffer object for compute shader access
     * @return Const reference to the index texture buffer object (TBO)[1,4](@ref)
     */
    const TBO& GetIndexTBO() const noexcept;

    /**
     * @brief Gets the number of triangles in the mesh
     * @return Count of triangles in the mesh[1,4](@ref)
     */
    unsigned int GetNumTriangles() const noexcept;

    /**
     * @brief Gets the number of vertices in the mesh
     * @return Count of vertices in the mesh[1,4](@ref)
     */
    unsigned int GetNumVertices() const noexcept;

protected:
    /**
     * @brief Initializes texture buffer objects (TBOs) for GPU access
     *
     * This method creates and configures TBOs for vertex data, normals,
     * texture coordinates, and indices for use in compute shaders[1,3](@ref).
     */
    void InitializeTBOs();

protected:
    Transform transform;                     ///< Transformation applied to the mesh vertices
    std::vector<float> vertices;            ///< Array of vertex positions (x, y, z coordinates)
    std::vector<unsigned int> indices;      ///< Array of vertex indices forming triangles
    std::vector<float> normals;             ///< Array of vertex normal vectors (nx, ny, nz)
    std::vector<float> texcoords;           ///< Array of texture coordinates (u, v)

    // TBOs for compute shader access
    std::unique_ptr<TBO> vertex_tbo_;       ///< Texture buffer object for vertex data
    std::unique_ptr<TBO> normal_tbo_;       ///< Texture buffer object for normal data
    std::unique_ptr<TBO> texcoord_tbo_;     ///< Texture buffer object for texture coordinate data
    std::unique_ptr<TBO> index_tbo_;        ///< Texture buffer object for index data
};

NAMESPACE_END(dream)