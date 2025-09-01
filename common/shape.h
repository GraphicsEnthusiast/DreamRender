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
 * @brief Encoded triangle structure for GPU storage
 */
struct TriangleEncoded {
    Point3f p1, p2, p3;       ///< Vertex positions
    Vector3f n1, n2, n3;      ///< Vertex normals
    Point3f uv1, uv2, uv3;    ///< UV coordinates (z-component unused)

	//glm::vec3 emissive;       // Emissive parameters
	//glm::vec3 baseColor;      // Base color
	//glm::vec3 param1;         // (subsurface, metallic, specular)
	//glm::vec3 param2;         // (specularTint, roughness, anisotropic)
	//glm::vec3 param3;         // (sheen, sheenTint, clearcoat)
	//glm::vec3 param4;         // (clearcoatGloss, IOR, transmission)
	//glm::vec3 tex;            // (isTex, texID, lightID)
};

/**
 * @class TriangleMeshManager
 * @brief Manages triangle mesh data and GPU resources
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
     * @param materials Material properties array (one per mesh)
     */
    void EncodeTriangles(const std::vector<TriangleMesh>& meshes/*, const std::vector<Material>& materials*/);

    /**
     * @brief Creates GPU buffers for encoded data
     */
    void CreateTBO();

    /**
     * @brief Gets GPU buffers
     */
    const TBO& GetTBO() const noexcept;

    /**
     * @brief Gets the number of triangles
     * @return Number of triangles
     */
    unsigned int GetNumTriangles() const noexcept;

    /**
     * @brief Gets the encoded triangles
     * @return Encoded triangles vector
     */
    std::vector<TriangleEncoded>& GetTriangleEncoded();

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
    std::unique_ptr<TBO> tbo_;                             ///< Texture buffer object for GPU storage
    static std::unique_ptr<TriangleMeshManager> instance_; ///< Singleton instance pointer
};

NAMESPACE_END(dream)