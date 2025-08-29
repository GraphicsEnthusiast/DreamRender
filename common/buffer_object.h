#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class BufferObject
 * @brief Base class for OpenGL buffer objects (VBO, TBO, etc.)
 */
class BufferObject {
public:
    /**
     * @brief Constructs and initializes a buffer object
     * @param target OpenGL buffer target (e.g., GL_ARRAY_BUFFER, GL_TEXTURE_BUFFER)
     * @param usage Buffer usage pattern (e.g., GL_STATIC_DRAW)
     */
    BufferObject(GLenum target, GLenum usage = GL_STATIC_DRAW) : id_(0), target_(target), usage_(usage), size_(0) {}

    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~BufferObject();

    /**
     * @brief Checks if the buffer is valid (has been generated)
     * @return True if the buffer has a non-zero ID
     */
    bool IsValid() const noexcept;

    /**
     * @brief Gets the OpenGL buffer ID
     * @return The buffer object ID
     */
    GLuint GetID() const noexcept;

    /**
     * @brief Gets the buffer target type
     * @return The OpenGL target this buffer is bound to
     */
    GLenum GetTarget() const noexcept;

    /**
     * @brief Gets the buffer size in bytes
     * @return Size of the buffer data allocation
     */
    GLsizeiptr GetSize() const noexcept;

protected:
    /**
     * @brief Generates a new OpenGL buffer object
     */
    void Generate();

    /**
     * @brief Binds the buffer to its target
     */
    void Bind() const;

    /**
     * @brief Unbinds any buffer from the target
     */
    void Unbind() const;

    /**
     * @brief Allocates and initializes buffer data storage
     * @param size Size of the data in bytes
     * @param data Pointer to the data to copy (nullptr for uninitialized allocation)
     */
    void BufferData(GLsizeiptr size, const void* data = nullptr);

    /**
     * @brief Updates a portion of the buffer data
     * @param offset Offset into the buffer where to start updating
     * @param size Size of the data portion in bytes
     * @param data Pointer to the new data
     */
    void BufferSubData(GLintptr offset, GLsizeiptr size, const void* data);

    /**
     * @brief Deletes the buffer and releases OpenGL resources
     */
    void Delete();

protected:
    GLuint id_;          ///< OpenGL buffer object ID
    GLenum target_;      ///< Buffer target (e.g., GL_ARRAY_BUFFER)
    GLenum usage_;       ///< Buffer usage pattern
    GLsizeiptr size_;    ///< Size of the buffer in bytes
};

/**
 * @class TextureBufferObject
 * @brief Represents an OpenGL Texture Buffer Object (TBO)(TBOs allow a buffer object to be accessed as a one-dimensional texture in shaders.
 *  They provide much larger capacity than traditional textures but lack filtering and other sampling features.)
 */
class TextureBufferObject : public BufferObject {
public:
    /**
     * @brief Constructs and initializes a TBO with data
     * @param data Pointer to the data to populate the buffer with
     * @param size Size of the data in bytes
     * @param internalFormat Internal texture format (e.g., GL_RGB32F)
     * @param usage Buffer usage pattern
     */
    TextureBufferObject(const void* data, GLsizeiptr size, GLenum internalFormat = GL_RGB32F, GLenum usage = GL_STATIC_DRAW);

    /**
     * @brief Destructor (cleans up both buffer and texture)
     */
    ~TextureBufferObject() override;

    /**
     * @brief Initializes the TBO with data and format
     * @param data Pointer to the data to populate the buffer with
     * @param size Size of the data in bytes
     * @param internalFormat Internal texture format
     * @param usage Buffer usage pattern
     */
    void Initialize(const void* data, GLsizeiptr size, GLenum internalFormat = GL_RGB32F, GLenum usage = GL_STATIC_DRAW);

    /**
     * @brief Binds the texture buffer to a texture unit
     * @param unit Texture unit to bind to (defaults to 0)
     */
    void BindTexture(GLuint unit = 0) const;

    /**
     * @brief Unbinds the texture buffer from a texture unit
     * @param unit Texture unit to unbind from
     */
    void UnbindTexture(GLuint unit = 0) const;

    /**
     * @brief Gets the internal texture format
     * @return The OpenGL internal format used for the texture
     */
    GLenum InternalFormat() const { return internal_format_; }

    /**
     * @brief Gets the texture ID
     * @return The OpenGL texture ID
     */
    GLuint TextureID() const { return texture_id_; }

protected:
    GLuint texture_id_;           ///< OpenGL texture object ID
    GLenum internal_format_;      ///< Internal texture format
};
using TBO = TextureBufferObject;

NAMESPACE_END(dream)