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
     * @brief Gets the TBO containing mesh light alias table data
     * @return Reference to the Texture Buffer Object
     */
    const TBO& GetMeshLightAliasTableTBO() const noexcept;

    /**
     * @brief Gets the sum of the mesh light table
     * @return The sum value as a float
     */
    float GetMeshLightTableSum() const noexcept;

    /**
     * @brief Gets the max of the mesh light table
     * @return The max value as a float
     */
    float GetMeshLightTableMax() const noexcept;

    /**
     * @brief Gets the size of the mesh light table
     * @return The size as an integer
     */
    unsigned int GetMeshLightTableSize() const noexcept;

    /**
     * @brief Gets the OpenGL texture array ID
     */
    GLuint GetTextureArray() const noexcept;

    /**
     * @brief Gets the number of loaded textures
     */
    int GetTextureCount() const noexcept;

    /**
     * @brief Loads a texture from file and resizes to 2048x2048
     * @param file_path Path to texture file
     * @param type Texture type
     * @return Texture ID, or -1 if failed
     */
    int LoadTexture(const std::string& file_path, TextureType type);

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
    SceneManager() : texture_array_(0) {}

    /**
     * @brief Builds alias table for light triangles considering area and emission
     */
    void BuildLightAliasTable();

    /**
     * @brief Builds BVH for a specific set of triangles
     * @param triangles Input triangles to build BVH for
     * @param bvh_nodes Output BVH nodes
     * @param is_light Whether input triangles is light
     * @return Vector of sorted triangles based on BVH construction
     */
    std::vector<TriangleEncoded> BuildBVHForTriangles(const std::vector<TriangleEncoded>& triangles, std::vector<BVHNodeEncoded>& bvh_nodes,
        bool is_light);

    /**
     * @brief Bilinear sampling for texture resizing
     * @param data Source texture data
     * @param width Source width
     * @param height Source height
     * @param channels Number of channels
     * @param u Normalized u coordinate
     * @param v Normalized v coordinate
     * @return Sampled color
     */
    glm::vec4 BilinearSample(const float* data, int width, int height, int channels, float u, float v) const;

    /**
     * @brief Creates texture array from loaded textures
     */
    void CreateTextureArray();

protected:
    std::vector<TriangleEncoded> triangles_encoded_;        ///< Encoded regular triangle data
    std::vector<TriangleEncoded> triangles_light_encoded_;  ///< Encoded light triangle data
    std::vector<BVHNodeEncoded> bvh_nodes_encoded_;         ///< BVH nodes for regular geometry
    std::vector<BVHNodeEncoded> bvh_nodes_light_encoded_;   ///< BVH nodes for light geometry

    std::vector<Texture> textures_;                         ///< Loaded textures
    GLuint texture_array_;                                  ///< OpenGL 2D texture array
    std::unordered_map<std::string, int> texture_name_to_id_; ///< Texture name to ID mapping

    AliasTable1D mesh_light_alias_table_;                   ///< Alias table for light triangle sampling
    std::vector<float> light_triangle_weights_;             ///< Weight for each light triangle (area + luminance)

    std::unique_ptr<TBO> triangle_tbo_;                     ///< TBO for regular triangle data
    std::unique_ptr<TBO> triangle_light_tbo_;               ///< TBO for light triangle data
    std::unique_ptr<TBO> bvh_node_tbo_;                     ///< TBO for regular BVH node data
    std::unique_ptr<TBO> bvh_node_light_tbo_;               ///< TBO for light BVH node data
    std::unique_ptr<TBO> mesh_light_alias_table_tbo_;       ///< TBO for alias table data

    static std::unique_ptr<SceneManager> instance_;         ///< Singleton instance pointer
};

NAMESPACE_END(dream)