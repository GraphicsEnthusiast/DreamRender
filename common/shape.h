#pragma once

#include <buffer_object.h>
#include <transform.h>

NAMESPACE_BEGIN(dream)

/**
 * @enum TextureType
 * @brief Enumerates texture types used in material systems
 */
enum class TextureType {
    EMISSION = 0,
    DIFFUSE = 1,    ///< Diffuse/albedo texture (base color)
    ROUGHNESS = 2,  ///< Roughness texture
};

/**
 * @struct Texture
 * @brief Represents a 2D texture with pixel data
 */
struct Texture {
    std::string path;                ///< Texture file path
    int width;                       ///< Texture width in pixels
    int height;                      ///< Texture height in pixels
    int channels;                    ///< Number of color channels (3=RGB, 4=RGBA)
    std::vector<float> data;         ///< Texture pixel data (float format)
    int texture_array_layer;         ///< Layer index in texture array (-1 if not in array)

    /**
     * @brief Checks if texture is valid
     */
    bool IsValid() const noexcept;

    /**
     * @brief Gets the size of texture data in bytes
     */
    unsigned int GetDataSize() const noexcept;

    /**
     * @brief Gets the total number of pixels
     */
    int GetPixelCount() const noexcept;
};

/**
 * @enum MaterialType
 * @brief Enumerates material types used in rendering
 */
enum class MaterialType {
    DIFFUSE = 0,   ///< Diffuse material (Oren-Nayar model)
};

/**
 * @struct Material
 * @brief Represents a material with textures
 */
struct Material {
    MaterialType type;
    std::unordered_map<TextureType, int> texture_ids; ///< Texture ID for each texture type
    Vector3f diffuse;                                 ///< Diffuse color (when no texture)
    float roughness;                                  ///< Roughness value (0.0-1.0)
    Vector3f emission;                                ///< Emission strength

    /**
     * @brief Default constructor
     */
    Material();

    /**
     * @brief Checks if material has a specific texture type
     * @param type Texture type to check
     * @return True if texture exists, false otherwise
     */
    bool HasTexture(TextureType type) const;

    /**
     * @brief Gets texture ID for a specific texture type
     * @param type Texture type
     * @return Texture ID, or -1 if not found
     */
    int GetTextureID(TextureType type) const;

    /**
     * @brief Sets texture ID for a specific texture type
     * @param type Texture type
     * @param texture_id Texture ID to set
     */
    void SetTexture(TextureType type, int texture_id);

    /**
     * @brief Removes texture for a specific texture type
     * @param type Texture type
     */
    void RemoveTexture(TextureType type);

    /**
     * @brief Clears all textures from the material
     */
    void ClearTextures();

    /**
     * @brief Gets the number of textures in the material
     * @return Number of textures
     */
    unsigned int GetTextureCount() const noexcept;

    /**
     * @brief Checks if the material uses textures
     * @return True if material has at least one texture, false otherwise
     */
    bool HasTextures() const noexcept;
};

enum class PhaseType {
    HenyeyGreenstein = 0,  ///< Henyey-Greenstein phase function
};

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
     * @param material Material for this mesh
     */
    TriangleMesh(const std::string& file, const Transform& trans, const Material& material = Material());

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

    /**
     * @brief Gets the material for this mesh
     * @return Const reference to the material
     */
    const Material& GetMaterial() const noexcept;

    /**
     * @brief Sets the material for this mesh
     * @param material Material to set
     */
    void SetMaterial(const Material& material);

protected:
    Transform transform_;                    ///< Transformation applied to the mesh vertices
    std::vector<float> vertices_;            ///< Array of vertex positions (x, y, z coordinates)
    std::vector<unsigned int> indices_;      ///< Array of vertex indices forming triangles
    std::vector<float> normals_;             ///< Array of vertex normal vectors (nx, ny, nz)
    std::vector<float> texcoords_;           ///< Array of texture coordinates (u, v)
    Material material_;                      ///< Material for this mesh
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

    alignas(16) Vector4f material_type; ///< Only x is useful, representing the material type, such as 0 for diffuse
    alignas(16) Vector4f emission;      ///< Emission (xyz components) and texture flag (w component: -1 = constant color, other = texture)
    alignas(16) Vector4f diffuse;       ///< Diffuse color (xyz components) and texture flag (w component: -1 = constant color, other = texture)
    alignas(16) Vector4f roughness;     ///< Roughness (x components) and texture flag (w component: -1 = constant color, other = texture)
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

NAMESPACE_END(dream)