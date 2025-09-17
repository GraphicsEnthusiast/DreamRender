#pragma once

#include <shape.h>

NAMESPACE_BEGIN(dream)

/**
 * @class SceneManager
 * @brief Manages scene data and GPU resources
 */
class SceneManager {
public:
    /**
     * @brief Get the singleton instance
     * @return SceneManager& Reference to the singleton instance
     */
    static SceneManager& Instance();

    /**
     * @brief Releases all resources and destroys the singleton instance
     */
    static void Release();

    /**
     * @brief Encodes mesh data from TriangleMesh array into GPU-friendly format
     * @param meshes Array of TriangleMesh objects
     */
    void EncodeTriangles(const std::vector<TriangleMesh>& meshes);

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
    SceneManager(const SceneManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     */
    SceneManager& operator=(const SceneManager&) = delete;

protected:
    /**
     * @brief Default constructor
     */
    SceneManager() = default;

    /**
     * @brief Internal resource cleanup method
     */
    void ReleaseInstance();

protected:
    std::vector<TriangleEncoded> triangles_encoded_;       ///< Encoded triangle data
    std::vector<BVHNodeEncoded> bvh_nodes_encoded_;        ///< BVH nodes in GPU-friendly format
    std::unique_ptr<TBO> triangle_tbo_;                    ///< TBO for triangle data
    std::unique_ptr<TBO> bvh_node_tbo_;                    ///< TBO for BVH node data
    static std::unique_ptr<SceneManager> instance_;        ///< Singleton instance pointer
};

NAMESPACE_END(dream)