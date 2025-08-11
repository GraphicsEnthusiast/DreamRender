#pragma once

#include <utils.h>

enum class ShaderType {
	Rasterization,
	Computation
};

class Shader {
public:
	Shader(ShaderType type) : shader_type_(type), id_(0) {}

	ShaderType GetShaderType() const;
	void Use();
	void SetBool(const std::string& name, bool value) const;
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	template <int N>
	void SetVector(const std::string& name, const glm::vec<N, float, glm::packed_highp>& vec);
	template <int N>
	void SetMatrix(const std::string& name, const glm::mat<N, N, float, glm::packed_highp>& mat);

protected:
	void CheckCompileErrors(unsigned int shader, std::string type);

protected:
	unsigned int id_;
	ShaderType shader_type_;
};

extern template void Shader::SetVector<2>(const std::string&, const glm::vec2&);
extern template void Shader::SetVector<3>(const std::string&, const glm::vec3&);
extern template void Shader::SetVector<4>(const std::string&, const glm::vec4&);

extern template void Shader::SetMatrix<2>(const std::string&, const glm::mat2&);
extern template void Shader::SetMatrix<3>(const std::string&, const glm::mat3&);
extern template void Shader::SetMatrix<4>(const std::string&, const glm::mat4&);

class RasterizationShader : public Shader {
public:
	RasterizationShader(const char* vertex_path, const char* fragment_path);
};

class ComputationShader : public Shader {
public:
	ComputationShader(const char* compute_path);
};
