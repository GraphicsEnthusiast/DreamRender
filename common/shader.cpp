#include <shader.h>

NAMESPACE_BEGIN(dream)

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
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream v_shader_file;
    std::ifstream f_shader_file;
    // Enable exception handling for file operations
    v_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // Open and read shader source files
        v_shader_file.open(vertex_path);
        f_shader_file.open(fragment_path);
        std::stringstream v_shader_stream, f_shader_stream;
        v_shader_stream << v_shader_file.rdbuf();
        f_shader_stream << f_shader_file.rdbuf();
        v_shader_file.close();
        f_shader_file.close();
        vertex_code = v_shader_stream.str();
        fragment_code = f_shader_stream.str();
    }
    catch (std::ifstream::failure e) {
        ERROR("[error] shader file not succesfully read.");
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
    // Cleanup individual shaders after linking
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

ComputationShader::ComputationShader(const char* compute_path) : Shader(ShaderType::Computation) {
    std::string compute_code;
    std::ifstream c_shader_file;
    // Enable exception handling for file operations
    c_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // Open and read compute shader source
        c_shader_file.open(compute_path);
        std::stringstream c_shader_stream;
        c_shader_stream << c_shader_file.rdbuf();
        c_shader_file.close();
        compute_code = c_shader_stream.str();
    }
    catch (std::ifstream::failure e) {
        ERROR("[error] shader file not succesfully read.");
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
    // Cleanup compute shader after linking
    glDeleteShader(compute);
}

NAMESPACE_END(dream)