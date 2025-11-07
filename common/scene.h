#pragma once

#include <shape.h>
#include <sampling.h>

NAMESPACE_BEGIN(dream)

/**
 * @class SceneManager
 * @brief Manages scene data and GPU resources with separate handling for light and regular geometry
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
     * @param is_light Whether array of TriangleMesh objects is light
     */
    void EncodeTriangles(const std::vector<TriangleMesh>& meshes, bool is_light);

    /**
     * @brief Builds separate BVH acceleration structures for regular and light triangles
     */
    void BuildBVH();

    /**
     * @brief Creates GPU buffers for encoded data and BVH structures
     */
    void CreateGPUBuffers();

    /**
     * @brief Gets the TBO containing regular triangle data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetTriangleTBO() const noexcept;

    /**
     * @brief Gets the TBO containing regular BVH node data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetBVHNodeTBO() const noexcept;

    /**
     * @brief Gets the TBO containing light triangle data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetTriangleLightTBO() const noexcept;

    /**
     * @brief Gets the TBO containing light BVH node data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetBVHNodeLightTBO() const noexcept;

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

    /**
     * @brief Builds BVH for a specific set of triangles
     * @param triangles Input triangles to build BVH for
     * @param bvh_nodes Output BVH nodes
     * @return Vector of sorted triangles based on BVH construction
     */
    std::vector<TriangleEncoded> BuildBVHForTriangles(
        const std::vector<TriangleEncoded>& triangles,
        std::vector<BVHNodeEncoded>& bvh_nodes);

protected:
    std::vector<TriangleEncoded> triangles_encoded_;        ///< Encoded regular triangle data
    std::vector<TriangleEncoded> triangles_light_encoded_;  ///< Encoded light triangle data
    std::vector<BVHNodeEncoded> bvh_nodes_encoded_;         ///< BVH nodes for regular geometry
    std::vector<BVHNodeEncoded> bvh_nodes_light_encoded_;   ///< BVH nodes for light geometry

    std::unique_ptr<TBO> triangle_tbo_;                     ///< TBO for regular triangle data
    std::unique_ptr<TBO> triangle_light_tbo_;               ///< TBO for light triangle data
    std::unique_ptr<TBO> bvh_node_tbo_;                     ///< TBO for regular BVH node data
    std::unique_ptr<TBO> bvh_node_light_tbo_;               ///< TBO for light BVH node data

    static std::unique_ptr<SceneManager> instance_;         ///< Singleton instance pointer
};

NAMESPACE_END(dream)