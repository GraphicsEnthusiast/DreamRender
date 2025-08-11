#include <shader.h>

ShaderType Shader::GetShaderType() const noexcept {
	return shader_type_;
}

void Shader::Use() {
	glUseProgram(id_);
}

void Shader::SetBool(const std::string& name, bool value) {
	glUniform1i(glGetUniformLocation(id_, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) {
	glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) {
	glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

template<int N>
void Shader::SetVector(const std::string& name, const glm::vec<N, float, glm::packed_highp>& vec) {
	GLint location = glGetUniformLocation(id_, name.c_str());
	if constexpr (N == 2) {
		glUniform2fv(location, 1, glm::value_ptr(vec));
	}
	else if constexpr (N == 3) {
		glUniform3fv(location, 1, glm::value_ptr(vec));
	}
	else if constexpr (N == 4) {
		glUniform4fv(location, 1, glm::value_ptr(vec));
	}
}

template<int N>
void Shader::SetMatrix(const std::string& name, const glm::mat<N, N, float, glm::packed_highp>& mat) {
	GLint location = glGetUniformLocation(id_, name.c_str());
	if constexpr (N == 2) {
		glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(mat));
	}
	else if constexpr (N == 3) {
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat));
	}
	else if constexpr (N == 4) {
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
	}
}

template void Shader::SetVector<2>(const std::string&, const glm::vec2&);
template void Shader::SetVector<3>(const std::string&, const glm::vec3&);
template void Shader::SetVector<4>(const std::string&, const glm::vec4&);

template void Shader::SetMatrix<2>(const std::string&, const glm::mat2&);
template void Shader::SetMatrix<3>(const std::string&, const glm::mat3&);
template void Shader::SetMatrix<4>(const std::string&, const glm::mat4&);

void Shader::CheckCompileErrors(unsigned int shader, const std::string& type) {
	int success;
	char info_log[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 1024, NULL, info_log);
			const std::string error = "[error] shader compilation error of type: " + type + "; " + std::string(info_log) + ".";
			ERROR(error);
		}
	}
	else {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader, 1024, NULL, info_log);
			const std::string error = "[error] shader compilation error of type: " + type + "; " + std::string(info_log) + ".";
			ERROR(error);
		}
	}
}

RasterizationShader::RasterizationShader(const char* vertex_path, const char* fragment_path) : Shader(ShaderType::Rasterization) {
	// 1. retrieve the vertex/fragment source code from file_path
	std::string vertex_code;
	std::string fragment_code;
	std::ifstream v_shader_file;
	std::ifstream f_shader_file;
	// ensure ifstream objects can throw exceptions:
	v_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		// open files
		v_shader_file.open(vertex_path);
		f_shader_file.open(fragment_path);
		std::stringstream v_shader_stream, f_shader_stream;
		// read file's buffer contents into streams
		v_shader_stream << v_shader_file.rdbuf();
		f_shader_stream << f_shader_file.rdbuf();
		// close file handlers
		v_shader_file.close();
		f_shader_file.close();
		// convert stream into string
		vertex_code = v_shader_stream.str();
		fragment_code = f_shader_stream.str();
	}
	catch (std::ifstream::failure e) {
		ERROR("[error] shader file not succesfully read.");
	}
	const char* v_shader_code = vertex_code.c_str();
	const char* f_shader_code = fragment_code.c_str();
	// 2. compile shaders
	unsigned int vertex, fragment;
	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &v_shader_code, NULL);
	glCompileShader(vertex);
	CheckCompileErrors(vertex, "VERTEX");
	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &f_shader_code, NULL);
	glCompileShader(fragment);
	CheckCompileErrors(fragment, "FRAGMENT");
	// shader Program
	id_ = glCreateProgram();
	glAttachShader(id_, vertex);
	glAttachShader(id_, fragment);
	glLinkProgram(id_);
	CheckCompileErrors(id_, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

ComputationShader::ComputationShader(const char* compute_path) : Shader(ShaderType::Computation) {
	std::string compute_code;
	std::ifstream c_shader_file;
	// ensure ifstream objects can throw exceptions:
	c_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		// open files
		c_shader_file.open(compute_path);
		std::stringstream c_shader_stream;
		// read file's buffer contents into streams
		c_shader_stream << c_shader_file.rdbuf();

		// close file handlers
		c_shader_file.close();
		// convert stream into string
		compute_code = c_shader_stream.str();
	}
	catch (std::ifstream::failure e) {
		ERROR("[error] shader file not succesfully read.");
	}
	const char* c_shader_code = compute_code.c_str();
	// 2. compile shaders
	unsigned int compute;
	// vertex shader
	compute = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute, 1, &c_shader_code, NULL);
	glCompileShader(compute);
	CheckCompileErrors(compute, "COMPUTE");
	// shader Program
	id_ = glCreateProgram();
	glAttachShader(id_, compute);
	glLinkProgram(id_);
	CheckCompileErrors(id_, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(compute);
}
