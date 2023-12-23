#pragma once

#include <string>

class Shader {
public:
	Shader(const char* vertexPath, const char* fragmentPath);

	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath);

private:
	void CheckCompileErrors(unsigned int id, std::string type);

public:
	std::string vertexString;
	std::string fragmentString;
	std::string geometryString;
	const char* vertexSource;
	const char* fragmentSource;
	const char* geometrySource;
	unsigned int ID;
};

