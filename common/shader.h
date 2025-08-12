#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @enum ShaderType
 * @brief Defines supported shader program types
 */
enum class ShaderType {
    Rasterization,  ///< Traditional rendering pipeline (vertex+fragment)
    Computation     ///< GPU computing pipeline (compute shader)
};

/**
 * @class Shader
 * @brief Base class for shader program management
 */
class Shader {
public:
    /**
     * @brief Constructs shader with specified type
     * @param type ShaderType classification
     */
    Shader(ShaderType type) : shader_type_(type), id_(0) {}

    ShaderType GetShaderType() const noexcept;  ///< Returns shader type
    void Use();  ///< Activates shader program
    void SetBool(const std::string& name, bool value);  ///< Sets boolean uniform
    void SetInt(const std::string& name, int value);  ///< Sets integer uniform
    void SetFloat(const std::string& name, float value);  ///< Sets float uniform

    /**
     * @brief Sets GLSL vector uniform
     * @tparam N Vector dimension (2/3/4)
     * @param name Uniform name
     * @param vec Vector value
     */
    template <int N>
    void SetVector(const std::string& name, const glm::vec<N, float, glm::packed_highp>& vec);

    /**
     * @brief Sets GLSL matrix uniform
     * @tparam N Matrix dimension (2/3/4)
     * @param name Uniform name
     * @param mat Matrix value
     */
    template <int N>
    void SetMatrix(const std::string& name, const glm::mat<N, N, float, glm::packed_highp>& mat);

protected:
    /**
     * @brief Checks compilation/linking errors
     * @param shader Shader/program ID
     * @param type Shader stage identifier
     */
    void CheckCompileErrors(unsigned int shader, const std::string& type);

protected:
    unsigned int id_;          ///< OpenGL program ID
    ShaderType shader_type_;   ///< Shader type classification
};

// Explicit template instantiation declarations
extern template void Shader::SetVector<2>(const std::string&, const glm::vec2&);
extern template void Shader::SetVector<3>(const std::string&, const glm::vec3&);
extern template void Shader::SetVector<4>(const std::string&, const glm::vec4&);

extern template void Shader::SetMatrix<2>(const std::string&, const glm::mat2&);
extern template void Shader::SetMatrix<3>(const std::string&, const glm::mat3&);
extern template void Shader::SetMatrix<4>(const std::string&, const glm::mat4&);

/**
 * @class RasterizationShader
 * @brief Manages traditional vertex+fragment rendering pipelines
 */
class RasterizationShader : public Shader {
public:
    /**
     * @brief Constructs from vertex/fragment shader files
     * @param vertex_path Vertex shader file path
     * @param fragment_path Fragment shader file path
     */
    RasterizationShader(const char* vertex_path, const char* fragment_path);
};

/**
 * @class ComputationShader
 * @brief Manages compute shader programs
 */
class ComputationShader : public Shader {
public:
    /**
     * @brief Constructs from compute shader file
     * @param compute_path Compute shader file path
     */
    ComputationShader(const char* compute_path);
};

NAMESPACE_END(dream)