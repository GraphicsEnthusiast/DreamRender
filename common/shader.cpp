#include <shader.h>

NAMESPACE_BEGIN(dream)

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

std::string Shader::PreprocessShader(const std::string& source, const std::string& base_path) {
	std::istringstream stream(source);
	std::ostringstream output;
	std::string line;
	static std::set<std::string> included_files;

	while (std::getline(stream, line)) {
		// Handle #include directives
		if (0 == line.find("#include")) {
			unsigned int start = line.find('"');
			unsigned int end = line.rfind('"');

			if (start != std::string::npos && end != std::string::npos && start < end) {
				std::string include_file = line.substr(start + 1, end - start - 1);
				std::string full_path = base_path + include_file;

				// Check for circular includes
				if (included_files.find(full_path) != included_files.end()) {
					output << "// [warning] Skipping duplicate include: "
						<< include_file << "\n";
					continue;
				}

				included_files.insert(full_path);

				// Read included file
				std::ifstream include_stream(full_path);
				if (!include_stream.is_open()) {
					ERROR("[error] Failed to open included shader file: " + full_path);
					continue;
				}

				std::string include_content(
					(std::istreambuf_iterator<char>(include_stream)),
					std::istreambuf_iterator<char>()
				);

				// Recursively process includes
				std::string processed_include = PreprocessShader(include_content, base_path);
				output << processed_include << "\n";
			}
			else {
				ERROR("[error] Invalid #include syntax: " + line);
				output << line << "\n";
			}
		}
		else {
			output << line << "\n";
		}
	}

	return output.str();
}

std::string Shader::ExtractBasePath(const std::string& file_path) {
	unsigned int found = file_path.find_last_of("/\\");

	return (found != std::string::npos) ? file_path.substr(0, found + 1) : "";
}

RasterizationShader::RasterizationShader(const char* vertex_path, const char* fragment_path) {
	std::string vertex_code;
	std::string fragment_code;
	std::ifstream v_shader_file;
	std::ifstream f_shader_file;

	v_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		// Extract base paths for includes
		std::string vertex_base_path = ExtractBasePath(vertex_path);
		std::string fragment_base_path = ExtractBasePath(fragment_path);

		// Read and preprocess vertex shader
		v_shader_file.open(vertex_path);
		std::stringstream v_shader_stream;
		v_shader_stream << v_shader_file.rdbuf();
		v_shader_file.close();
		vertex_code = PreprocessShader(v_shader_stream.str(), vertex_base_path);

		// Read and preprocess fragment shader
		f_shader_file.open(fragment_path);
		std::stringstream f_shader_stream;
		f_shader_stream << f_shader_file.rdbuf();
		f_shader_file.close();
		fragment_code = PreprocessShader(f_shader_stream.str(), fragment_base_path);
	}
	catch (std::ifstream::failure& e) {
		ERROR("[error] shader file not successfully read: " + std::string(e.what()));
	}

	const char* v_shader_code = vertex_code.c_str();
	const char* f_shader_code = fragment_code.c_str();
	unsigned int vertex, fragment;

	// Compile vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &v_shader_code, NULL);
	glCompileShader(vertex);
	CheckCompileErrors(vertex, "VERTEX");

	// Compile fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &f_shader_code, NULL);
	glCompileShader(fragment);
	CheckCompileErrors(fragment, "FRAGMENT");

	// Create and link shader program
	id_ = glCreateProgram();
	glAttachShader(id_, vertex);
	glAttachShader(id_, fragment);
	glLinkProgram(id_);
	CheckCompileErrors(id_, "PROGRAM");

	// Cleanup individual shaders
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

ComputationShader::ComputationShader(const char* compute_path) {
	std::string compute_code;
	std::ifstream c_shader_file;

	c_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		// Extract base path for includes
		std::string base_path = ExtractBasePath(compute_path);

		// Read and preprocess compute shader
		c_shader_file.open(compute_path);
		std::stringstream c_shader_stream;
		c_shader_stream << c_shader_file.rdbuf();
		c_shader_file.close();
		compute_code = PreprocessShader(c_shader_stream.str(), base_path);
	}
	catch (std::ifstream::failure& e) {
		ERROR("[error] Shader file not successfully read: " + std::string(e.what()));
	}

	const char* c_shader_code = compute_code.c_str();
	unsigned int compute;

	// Compile compute shader
	compute = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute, 1, &c_shader_code, NULL);
	glCompileShader(compute);
	CheckCompileErrors(compute, "COMPUTE");

	// Create and link shader program
	id_ = glCreateProgram();
	glAttachShader(id_, compute);
	glLinkProgram(id_);
	CheckCompileErrors(id_, "PROGRAM");

	// Cleanup compute shader
	glDeleteShader(compute);
}

NAMESPACE_END(dream)