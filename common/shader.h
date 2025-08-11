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
	inline void SetVector(const std::string& name, const glm::vec<N, float, glm::packed_highp>& vec) {
		switch (N) {
		case 2: {
			glUniform2fv(glGetUniformLocation(id, name.c_str()), vec);
		};
			  break;
		case 3: {
			glUniform3fv(glGetUniformLocation(id, name.c_str()), vec);
		};
			  break;
		case 4: {
			glUniform4fv(glGetUniformLocation(id, name.c_str()), vec);
		};
			  break;
		}
	}

	template <int N>
	inline void SetMatrix(const std::string& name, const glm::mat<N, N, float, glm::packed_highp>& mat)
	{
		switch (N) {
		case 2: {
			glUniformMatrix2fv(glGetUniformLocation(id, name.c_str()), mat);
		};
			  break;
		case 3: {
			glUniformMatrix3fv(glGetUniformLocation(id, name.c_str()), mat);
		};
			  break;
		case 4: {
			glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), mat);
		};
			  break;
		}
	}

protected:
	void CheckCompileErrors(unsigned int shader, std::string type);

protected:
	unsigned int id_;
	ShaderType shader_type_;
};

class RasterizationShader : public Shader {
public:
	RasterizationShader(const char* vertex_path, const char* fragment_path);
};

class ComputationShader : public Shader {
public:
	ComputationShader(const char* compute_path);
};
