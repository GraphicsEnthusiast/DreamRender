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
     * @param trans Transformation to apply to the mesh vertices during loading
     */
    TriangleMesh(const std::string& file, const Transform& trans);

    /**
     * @brief Gets the number of triangles in the mesh
     * @return Count of triangles in the mesh
     */
    unsigned int GetNumTriangles() const noexcept;

    /**
     * @brief Gets the number of vertices in the mesh
     * @return Count of vertices in the mesh
     */
    unsigned int GetNumVertices() const noexcept;

    /**
     * @brief Gets the array of vertex positions (x, y, z coordinates)
     * @return Const reference to the vector of vertex positions
     */
    const std::vector<float>& GetVertices() const noexcept;

    /**
     * @brief Gets the array of vertex indices forming triangles
     * @return Const reference to the vector of vertex indices
     */
    const std::vector<unsigned int>& GetIndices() const noexcept;

    /**
     * @brief Gets the array of vertex normal vectors (nx, ny, nz)
     * @return Const reference to the vector of vertex normals
     */
    const std::vector<float>& GetNormals() const noexcept;

    /**
     * @brief Gets the array of texture coordinates (u, v)
     * @return Const reference to the vector of texture coordinates
     */
    const std::vector<float>& GetTexCoords() const noexcept;

protected:
    Transform transform_;                    ///< Transformation applied to the mesh vertices
    std::vector<float> vertices_;            ///< Array of vertex positions (x, y, z coordinates)
    std::vector<unsigned int> indices_;      ///< Array of vertex indices forming triangles
    std::vector<float> normals_;             ///< Array of vertex normal vectors (nx, ny, nz)
    std::vector<float> texcoords_;           ///< Array of texture coordinates (u, v)
};

/**
 * @struct TriangleEncoded
 * @brief Encoded triangle structure for GPU storage with packed UV coordinates
 */
struct alignas(16) TriangleEncoded {
	alignas(16) Point4f p1;    ///< Vertex positions (w stores uv1.x)
	alignas(16) Point4f p2;    ///< Vertex positions (w stores uv2.x)
	alignas(16) Point4f p3;    ///< Vertex positions (w stores uv3.x)
	alignas(16) Vector4f n1;   ///< Vertex normals (w stores uv1.y)
	alignas(16) Vector4f n2;   ///< Vertex normals (w stores uv2.y)
	alignas(16) Vector4f n3;   ///< Vertex normals (w stores uv3.y)
};

/**
 * @struct BVHNodeGPU
 * @brief GPU-friendly BVH node structure for efficient traversal
 */
struct alignas(16) BVHNodeEncoded {
    alignas(16) Point4f lmin;  ///< unsigned left child index in w component
    alignas(16) Point4f lmax;  ///< unsigned right child index in w component
    alignas(16) Point4f rmin;  ///< unsigned triangle count in w component
    alignas(16) Point4f rmax;  ///< unsigned first triangle index in w component
};

/**
 * @class TriangleMeshManager
 * @brief Manages triangle mesh data, BVH acceleration structure and GPU resources
 */
class TriangleMeshManager {
public:
    /**
     * @brief Get the singleton instance
     * @return TriangleMeshManager& Reference to the singleton instance
     */
    static TriangleMeshManager& Instance();

    /**
     * @brief Releases all resources and destroys the singleton instance
     */
    static void Release();

    /**
     * @brief Encodes mesh data from TriangleMesh array into GPU-friendly format
     * @param meshes Array of TriangleMesh objects
     */
    void BuildTriangles(const std::vector<TriangleMesh>& meshes);

    /**
     * @brief Builds BVH acceleration structure for the encoded triangles
     */
    void BuildBVH();

    /**
     * @brief Creates GPU buffers for encoded data and BVH structure
     */
    void CreateGPUBuffers();

    /**
     * @brief Gets the TBO containing triangle data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetTriangleTBO() const noexcept;

    /**
     * @brief Gets the TBO containing BVH node data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetBVHNodeTBO() const noexcept;

    /**
     * @brief Deleted copy constructor
     */
    TriangleMeshManager(const TriangleMeshManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     */
    TriangleMeshManager& operator=(const TriangleMeshManager&) = delete;

protected:
    /**
     * @brief Default constructor
     */
    TriangleMeshManager() = default;

    /**
     * @brief Internal resource cleanup method
     */
    void ReleaseInstance();

protected:
    std::vector<TriangleEncoded> triangles_encoded_;       ///< Encoded triangle data
    std::vector<BVHNodeEncoded> bvh_nodes_encoded_;        ///< BVH nodes in GPU-friendly format
    std::unique_ptr<TBO> triangle_tbo_;                    ///< TBO for triangle data
    std::unique_ptr<TBO> bvh_node_tbo_;                    ///< TBO for BVH node data
    static std::unique_ptr<TriangleMeshManager> instance_; ///< Singleton instance pointer
};

NAMESPACE_END(dream)