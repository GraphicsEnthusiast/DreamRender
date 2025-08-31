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

/**
 * @struct BVHNode
 * @brief Represents a node in a Bounding Volume Hierarchy (BVH) acceleration structure
 */
struct BVHNode {
    int left = -1;                ///< Left child index (-1 indicates a leaf node)
    int right = -1;               ///< Right child index (-1 indicates a leaf node)
    int n = 0;                    ///< Number of triangles stored in this leaf node (0 for internal nodes)
    int index = 0;                ///< First triangle index in the leaf (only valid for leaf nodes)
	Point3f aa = Point3f(MaxFloat);         ///< Minimum point (x, y, z) of the axis-aligned bounding box (AABB)
	Point3f bb = Point3f(MinFloat);         ///< Maximum point (x, y, z) of the axis-aligned bounding box (AABB)
};

/**
 * @struct BVHNodeEncoded
 * @brief GPU-optimized BVH node structure with packed data for efficient memory access
 */
struct BVHNodeEncoded {
    Point3f children;       ///< Packed child indices: (left, right, reserved)
    Point3f leaf_info;      ///< Packed leaf information: (n, index, reserved)
    Point3f aa;             ///< Minimum point of the axis-aligned bounding box
    Point3f bb;             ///< Maximum point of the axis-aligned bounding box
};

/**
 * @struct TriangleSorter
 * @brief Functor for sorting triangles along a specific axis based on their centroids
 */
struct TriangleSorter {
    int axis;                                   ///< Axis along which to sort (0=x, 1=y, 2=z)
    const std::vector<TriangleEncoded>& triangles;     ///< Reference to the triangle data array

    /**
     * @brief Constructs a TriangleSorter functor
     * @param axis Sorting axis (0=x, 1=y, 2=z)
     * @param triangles Reference to the triangle data vector
     */
    TriangleSorter(int axis, const std::vector<TriangleEncoded>& triangles)
        : axis(axis), triangles(triangles) {}

    /**
     * @brief Compares two triangles by their centroid coordinates along the specified axis
     * @param a Index of the first triangle in the triangles array
     * @param b Index of the second triangle in the triangles array
     * @return True if triangle a's centroid is less than triangle b's centroid on the specified axis
     */
    inline bool operator()(int a, int b) const {
		Point3f centerA = (triangles[a].p1 + triangles[a].p2 + triangles[a].p3) / 3.0f;
		Point3f centerB = (triangles[b].p1 + triangles[b].p2 + triangles[b].p3) / 3.0f;

        return centerA[axis] < centerB[axis];
    }
};

class BVH {
public:
    /**
     * @brief Gets the singleton instance of the BVH
     * @return Reference to the singleton BVH instance
     */
    static BVH& Instance();

    /**
     * @brief Releases all resources and destroys the singleton instance
     */
    static void Release();

    /**
     * @brief Gets the number of nodes in the BVH
     * @return Number of nodes
     */
    int GetNumNodes() const noexcept;

    /**
     * @brief Gets GPU buffers
     */
    const TBO& GetTBO() const noexcept;

    /**
     * @brief Deleted copy constructor
     */
    BVH(const BVH&) = delete;

    /**
     * @brief Deleted copy assignment operator
     */
    BVH& operator=(const BVH&) = delete;

protected:
    /**
     * @brief Private constructor for singleton pattern
     */
    BVH() = default;

    /**
     * @brief Internal resource cleanup method
     */
    void ReleaseResources();

    /**
     * @brief Builds BVH using Surface Area Heuristic (SAH) in parallel
     * @param l Start index of triangle range
     * @param r End index of triangle range
     * @param n Maximum triangles per leaf node
     * @return Node index of constructed subtree
     */
    int BuildBVHwithSAH(int l, int r, int n);

    /**
     * @brief Computes bounding box for triangle range
     * @param start Start index
     * @param end End index
     * @param[out] aabbMin Output min point
     * @param[out] aabbMax Output max point
     */
    void ComputeAABB(int start, int end, Point3f& aabb_min, Point3f& aabb_max);

    /**
     * @brief SAH cost calculation for an axis in single-threaded mode
     * @param start Start index of triangle range
     * @param end End index of triangle range
     * @param axis Axis to evaluate (0=x, 1=y, 2=z)
     * @param best_cost Output best cost found
     * @param best_split Output best split position found
     */
    void ComputeSAHForAxis(int start, int end, int axis, float& best_cost, int& best_split);

    /**
     * @brief Transforms data to GPU-friendly format
     */
    void TransformBVHNode();

    /**
     * @brief Creates GPU texture buffer objects
     */
    void CreateTBO();

protected:
	int num_triangles_;                                  ///< Number of triangles in the BVH
	int num_nodes_;                                      ///< Number of nodes in the BVH hierarchy
	std::vector<BVHNode> nodes_;                         ///< CPU storage for BVH nodes
	std::vector<BVHNodeEncoded> nodes_encoded_;          ///< GPU-encoded BVH node data
    std::unique_ptr<TBO> tbo_;                           ///< Texture buffer object for GPU storage
	static std::unique_ptr<BVH> instance_;               ///< Singleton instance pointer
};


NAMESPACE_END(dream)