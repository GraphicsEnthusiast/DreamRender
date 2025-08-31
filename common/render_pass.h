#pragma once

#include <shader.h>
#include <shape.h>

NAMESPACE_BEGIN(dream)

/**
 * @struct TextureHandle
 * @brief Represents a handle to a GPU texture resource
 *
 * Uses integer-based identifier for lightweight texture referencing.
 * Provides equality comparison operator for container compatibility.
 */
struct TextureHandle {
    GLuint id = UINT32_MAX; ///< Unique texture identifier (UINT32_MAX = invalid handle)

    /**
     * @brief Equality comparison operator
     * @param other TextureHandle to compare against
     * @return true if texture IDs match
     */
    inline bool operator==(const TextureHandle& other) const {
        return other.id == id;
    }

    /**
     * @brief Checks if valid
     * @return true if valid
     */
    inline bool IsValid() const noexcept {
        return UINT32_MAX != id;
    }
};

/**
 * @struct ResourceEdge
 * @brief Defines explicit connection between render passes
 */
struct ResourceEdge {
    std::string src_pass;     ///< Source pass name
    std::string src_output;   ///< Source output slot name
    std::string dst_pass;     ///< Destination pass name
    std::string dst_input;    ///< Destination input slot name
};

/**
 * @class RenderPass
 * @brief Abstract base class for render passes with named resource slots
 */
class RenderPass {
public:
    /// Map type for input slot name to texture handle
    using InputSlotMap = std::unordered_map<std::string, TextureHandle>;

    /// Map type for output slot name to texture handle
    using OutputSlotMap = std::unordered_map<std::string, TextureHandle>;

    /**
     * @brief Constructor for polymorphic deletion
     */
    RenderPass() = default;

    /**
     * @brief Virtual destructor for polymorphic deletion
     */
    virtual ~RenderPass() = default;

    /**
     * @brief Pure virtual function for rendering command execution
     * @note Must be implemented by derived classes
     */
    virtual void Execute() = 0;

    /**
	 * @brief Sets the texture handle for a specified input slot.
	 * @param slot_name Name identifier of the input slot to modify.
	 * @param handle Texture handle to assign to the slot.
	 */
    void SetInputTexture(const std::string& slot_name, const TextureHandle& handle);

    /**
     * @brief Sets the texture handle for a specified output slot.
     * @param slot_name Name identifier of the output slot to modify.
     * @param handle Texture handle to assign to the slot.
     */
    void SetOutputTexture(const std::string& slot_name, const TextureHandle& handle);

    /**
     * @brief Retrieves the texture handle for a specified input slot.
     * @param slot_name Name identifier of the input slot to retrieve.
     * @return TextureHandle associated with the input slot (invalid if not found)
     */
    TextureHandle GetInputTexture(const std::string& slot_name) const noexcept;

    /**
     * @brief Retrieves the texture handle for a specified output slot.
     * @param slot_name Name identifier of the output slot to retrieve.
     * @return TextureHandle associated with the output slot (invalid if not found)
     */
    TextureHandle GetOutputTexture(const std::string& slot_name) const noexcept;

    /**
     * @brief Gets the current name of the object.
     * @return Current name of the object (guaranteed valid due to noexcept).
     */
    const std::string& GetName() const noexcept;

protected:
    InputSlotMap input_map_;             ///< Input slot name to resource mapping
    OutputSlotMap output_map_;           ///< Output slot name to resource mapping
    std::string name_;
};

/**
 * @class SimpleComputePass
 * @brief Demonstrates compute shader usage by generating a gradient texture
 */
class SimpleComputePass : public RenderPass {
public:
    SimpleComputePass() {
        name_ = "Simple compute pass";

        // Create compute shader
        const char* compute_path = "../shader/test.comp";

        shader_ = std::make_unique<ComputationShader>(compute_path);
    }

    void Execute() override {
		// Use compute shader
		shader_->Use();

        // Bind output texture
        glBindImageTexture(0, GetOutputTexture("Output").id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        // Set time uniform variable
        static float time = 0.0f;
        time += 0.01f;
        shader_->SetFloat("time", time);
        TriangleMeshManager::Instance().GetTBO().BindTexture(0);
        shader_->SetInt("triangles", 0);

        // Dispatch compute shader
        glDispatchCompute(512 / 16, 512 / 16, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

protected:
    std::unique_ptr<ComputationShader> shader_;
};

NAMESPACE_END(dream)