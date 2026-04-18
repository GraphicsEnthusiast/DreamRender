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
    BufferObject(GLenum target, GLenum usage = GL_STATIC_DRAW);

    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~BufferObject();

    /**
     * @brief Issues a memory barrier for buffer operations
     * @param barriers Bitfield specifying the types of operations to barrier against
     */
    static void Barrier(GLbitfield barriers);

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
    TextureBufferObject(const void* data, GLsizeiptr size, GLenum internal_format = GL_RGB32F, GLenum usage = GL_STATIC_DRAW);

    /**
     * @brief Destructor (cleans up both buffer and texture)
     */
    ~TextureBufferObject() override;

    /**
     * @brief Binds the texture buffer to a texture unit
     * @param unit Texture unit to bind to (defaults to 0)
     */
    void BindTexture(GLuint unit = 0) const;

protected:
    /**
     * @brief Initializes the TBO with data and format
     * @param data Pointer to the data to populate the buffer with
     * @param size Size of the data in bytes
     * @param internal_format Internal texture format
     * @param usage Buffer usage pattern
     */
    void Initialize(const void* data, GLsizeiptr size, GLenum internalFormat = GL_RGB32F, GLenum usage = GL_STATIC_DRAW);

protected:
    GLuint texture_id_;           ///< OpenGL texture object ID
    GLenum internal_format_;      ///< Internal texture format
};
using TBO = TextureBufferObject;

/**
 * @class ShaderStorageBufferObject
 * @brief Represents an OpenGL Shader Storage Buffer Object (SSBO)(SSBOs allow shaders to read from and write to 
 *  a buffer object. They are commonly used for complex data sharing between shader stages or between the 
 *  application and shaders, and are essential for GPGPU.)
 */
class ShaderStorageBufferObject : public BufferObject {
public:
    /**
     * @brief Constructs an SSBO with optional initial data
     * @param size Size of the buffer in bytes
     * @param data Pointer to initial data (nullptr for uninitialized)
     * @param usage Buffer usage pattern (e.g., GL_DYNAMIC_DRAW)
     * @param flags Buffer storage flags (e.g., GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)
     */
    ShaderStorageBufferObject(GLsizeiptr size, const void* data = nullptr,
        GLenum usage = GL_DYNAMIC_DRAW, GLbitfield flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

    /**
     * @brief Destructor
     */
    ~ShaderStorageBufferObject() override = default;

    /**
     * @brief Binds the SSBO to a specific binding point index
     * @param index The binding point index (corresponds to 'binding' in shader layout)
     */
    void BindBase(GLuint index) const;

     /**
      * @brief Maps the buffer's data store to the client's address space
      * @param access Access policy (e.g., GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE)
      * @return Pointer to the mapped data, or nullptr if failed
      */
    void* MapBuffer(GLenum access) const;

    /**
     * @brief Unmaps the previously mapped data store
     * @return True if successful, false otherwise
     */
    bool UnmapBuffer() const;

protected:
    /**
     * @brief Initializes the SSBO with immutable storage
     * @param size Size of the buffer in bytes
     * @param data Pointer to initial data
     * @param flags Buffer storage flags
     */
    void InitializeStorage(GLsizeiptr size, const void* data, GLbitfield flags);

protected:
    GLbitfield flags_; ///< Buffer storage creation flags
};
using SSBO = ShaderStorageBufferObject;

/**
 * @class UniformBufferObject
 * @brief Represents an OpenGL Uniform Buffer Object (UBO)
 */
class UniformBufferObject : public BufferObject {
public:
    /**
     * @brief Constructs a UBO with specified data
     * @param data Pointer to the uniform data
     * @param size Size of the uniform data in bytes
     * @param usage Buffer usage pattern (e.g., GL_DYNAMIC_DRAW for frequent updates)
     */
    UniformBufferObject(const void* data, GLsizeiptr size, GLenum usage = GL_DYNAMIC_DRAW);

    /**
     * @brief Constructs an empty UBO of specified size
     * @param size Size of the uniform buffer in bytes
     * @param usage Buffer usage pattern
     */
    UniformBufferObject(GLsizeiptr size, GLenum usage = GL_DYNAMIC_DRAW);

    /**
     * @brief Destructor
     */
    ~UniformBufferObject() override = default;

    /**
     * @brief Binds the UBO to a specific binding point index
     * @param index The binding point index (corresponds to 'binding' in shader layout)
     */
    void BindBase(GLuint index) const;

    /**
     * @brief Maps the buffer's data store to the client's address space
     * @param access Access policy (e.g., GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE)
     * @return Pointer to the mapped data, or nullptr if failed
     */
    void* MapBuffer(GLenum access) const;

    /**
     * @brief Unmaps the previously mapped data store
     * @return True if successful, false otherwise
     */
    bool UnmapBuffer() const;

    /**
     * @brief Updates a portion of the uniform buffer data
     * @param offset Offset into the buffer where to start updating
     * @param size Size of the data portion in bytes
     * @param data Pointer to the new data
     */
    void UpdateData(GLintptr offset, GLsizeiptr size, const void* data);

    /**
     * @brief Updates the entire uniform buffer with new data
     * @param data Pointer to the new data
     */
    void UpdateData(const void* data);

protected:
    /**
     * @brief Initializes the uniform buffer
     * @param data Pointer to initial data (nullptr for uninitialized)
     * @param size Size of the buffer in bytes
     * @param usage Buffer usage pattern
     */
    void Initialize(const void* data, GLsizeiptr size, GLenum usage);
};
using UBO = UniformBufferObject;

NAMESPACE_END(dream)