#include <buffer_object.h>

NAMESPACE_BEGIN(dream)

BufferObject::BufferObject(GLenum target, GLenum usage) : id_(0), target_(target), usage_(usage), size_(0) { 
	Generate(); 
}

BufferObject::~BufferObject() {
	Delete();
}

void BufferObject::Barrier(GLbitfield barriers) {
	glMemoryBarrier(barriers);
}

void BufferObject::Generate() {
	glGenBuffers(1, &id_);
}

void BufferObject::Bind() const {
	glBindBuffer(target_, id_);
}

void BufferObject::Unbind() const {
	glBindBuffer(target_, 0);
}

void BufferObject::BufferData(GLsizeiptr size, const void* data) {
	size_ = size;
	glBufferData(target_, size_, data, usage_);
}

void BufferObject::BufferSubData(GLintptr offset, GLsizeiptr size, const void* data) {
	glBufferSubData(target_, offset, size, data);
}

void BufferObject::Delete() {
	if (0 != id_) {
		glDeleteBuffers(1, &id_);
		id_ = 0;
		size_ = 0;
	}
}

TextureBufferObject::TextureBufferObject(const void* data, GLsizeiptr size, GLenum internal_format, GLenum usage)
	: BufferObject(GL_TEXTURE_BUFFER, usage), texture_id_(0), internal_format_(internal_format) {
	Bind();
	Initialize(data, size, internal_format, usage);
	Unbind();
}

TextureBufferObject::~TextureBufferObject() {
	if (0 != texture_id_) {
		glDeleteTextures(1, &texture_id_);
		texture_id_ = 0;
	}
}

void TextureBufferObject::Initialize(const void* data, GLsizeiptr size, GLenum internal_format, GLenum usage) {
	internal_format_ = internal_format;
	usage_ = usage;

	BufferData(size, data);

	// Generate and configure the texture
	glGenTextures(1, &texture_id_);
	glBindTexture(GL_TEXTURE_BUFFER, texture_id_);
	glTexBuffer(GL_TEXTURE_BUFFER, internal_format_, id_);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void TextureBufferObject::BindTexture(GLuint unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_BUFFER, texture_id_);
}

ShaderStorageBufferObject::ShaderStorageBufferObject(GLsizeiptr size, const void* data, GLenum usage, GLbitfield flags)
	: BufferObject(GL_SHADER_STORAGE_BUFFER, usage), flags_(flags) {
	Bind();
	InitializeStorage(size, data, flags_);
	Unbind();
}

void ShaderStorageBufferObject::BindBase(GLuint index) const {
	if (0 != id_) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id_);
	}
}

void* ShaderStorageBufferObject::MapBuffer(GLenum access) const {
	if (0 == id_) {
		return nullptr;
	}

	return glMapBuffer(target_, access);
}

bool ShaderStorageBufferObject::UnmapBuffer() const {
	if (0 == id_) {
		return false;
	}

	return GL_TRUE == glUnmapBuffer(target_);
}

void ShaderStorageBufferObject::InitializeStorage(GLsizeiptr size, const void* data, GLbitfield flags) {
	size_ = size;
	glBufferStorage(target_, size_, data, flags);
}

UniformBufferObject::UniformBufferObject(const void* data, GLsizeiptr size, GLenum usage)
	: BufferObject(GL_UNIFORM_BUFFER, usage) {
	Bind();
	Initialize(data, size, usage);
	Unbind();
}

UniformBufferObject::UniformBufferObject(GLsizeiptr size, GLenum usage)
	: BufferObject(GL_UNIFORM_BUFFER, usage) {
	Bind();
	Initialize(nullptr, size, usage);
	Unbind();
}

void UniformBufferObject::BindBase(GLuint index) const {
	if (0 != id_) {
		glBindBufferBase(GL_UNIFORM_BUFFER, index, id_);
	}
}

void* UniformBufferObject::MapBuffer(GLenum access) const {
	if (0 == id_) {
		return nullptr;
	}
	return glMapBuffer(target_, access);
}

bool UniformBufferObject::UnmapBuffer() const {
	if (0 == id_) {
		return false;
	}
	return GL_TRUE == glUnmapBuffer(target_);
}

void UniformBufferObject::UpdateData(GLintptr offset, GLsizeiptr size, const void* data) {
	Bind();
	BufferSubData(offset, size, data);
	Unbind();
}

void UniformBufferObject::UpdateData(const void* data) {
	Bind();
	BufferSubData(0, size_, data);
	Unbind();
}

void UniformBufferObject::Initialize(const void* data, GLsizeiptr size, GLenum usage) {
	usage_ = usage;
	BufferData(size, data);
}

NAMESPACE_END(dream)