#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Shader
 * @brief Base class for shader program management
 */
class Shader {
public:
    /**
     * @brief Constructs shader with specified type
     */
    Shader() : id_(0) {}

    /**
     * @brief Activates the shader program for rendering
     */
    void Use();

    /**
     * @brief Sets a boolean uniform value in the shader
     * @param name Name of the uniform variable
     * @param value Boolean value to set
     */
    void SetBool(const std::string& name, bool value);

    /**
     * @brief Sets an integer uniform value in the shader
     * @param name Name of the uniform variable
     * @param value Integer value to set
     */
    void SetInt(const std::string& name, int value);

    /**
     * @brief Sets a floating-point uniform value in the shader
     * @param name Name of the uniform variable
     * @param value Float value to set
     */
    void SetFloat(const std::string& name, float value);

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

    /**
	 * @brief Preprocesses shader source code with custom directives
	 * @param source Original shader source code string
	 * @param base_path Base directory path for resolving includes
	 * @return Preprocessed shader source code
	 */
    std::string PreprocessShader(const std::string& source, const std::string& base_path);

    /**
	 * @brief Extracts the base directory path from a full file path
	 * @param file_path Full path to a shader file (absolute or relative)
	 * @return Directory path component of the input file path
	 * @example
	 *   Input: "shaders/core/lighting.frag"
	 *   Output: "shaders/core/"
	 */
	std::string ExtractBasePath(const std::string& file_path);

protected:
    unsigned int id_;          ///< OpenGL program ID
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