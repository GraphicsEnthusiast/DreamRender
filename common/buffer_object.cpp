#include <buffer_object.h>

NAMESPACE_BEGIN(dream)

BufferObject::BufferObject(GLenum target, GLenum usage) : id_(0), target_(target), usage_(usage), size_(0) { 
	Generate(); 
}

BufferObject::~BufferObject() {
	if (0 != id_) {
		Delete();
	}
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

bool BufferObject::IsValid() const noexcept {
	return 0 != id_;
}

GLuint BufferObject::GetID() const noexcept {
	return id_;
}

GLenum BufferObject::GetTarget() const noexcept {
	return target_;
}

GLsizeiptr BufferObject::GetSize() const noexcept {
	return size_;
}

TextureBufferObject::TextureBufferObject(const void* data, GLsizeiptr size, GLenum internal_format, GLenum usage)
	: BufferObject(GL_TEXTURE_BUFFER, usage), texture_id_(0), internal_format_(internal_format) {
	Initialize(data, size, internal_format, usage);
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

	Bind();
	BufferData(size, data);

	// Generate and configure the texture
	glGenTextures(1, &texture_id_);
	glBindTexture(GL_TEXTURE_BUFFER, texture_id_);
	glTexBuffer(GL_TEXTURE_BUFFER, internal_format_, id_);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	Unbind();
}

void TextureBufferObject::BindTexture(GLuint unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_BUFFER, texture_id_);
}

void TextureBufferObject::UnbindTexture(GLuint unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

NAMESPACE_END(dream)