#pragma once

#include <shader.h>

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
     * @brief Constructs a RenderPass with specified enable state
     */
    RenderPass() : enabled_(true) {}

    /**
     * @brief Virtual destructor for polymorphic deletion
     */
    virtual ~RenderPass() = default;

    /**
     * @brief Sets the activation state of the render pass
     * @param enabled New activation state
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Checks current activation status
     * @return true if pass is enabled and should execute
     */
    bool IsEnabled() const noexcept;

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
     * @brief Retrieves all input slot names and their associated texture handles.
     * @return const reference to the input slot map
     */
    const InputSlotMap& GetInputSlots() const noexcept;

    /**
     * @brief Retrieves all output slot names and their associated texture handles.
     * @return const reference to the output slot map
     */
    const OutputSlotMap& GetOutputSlots() const noexcept;

protected:
    bool enabled_;                       ///< Controls pass execution
    InputSlotMap input_map_;             ///< Input slot name to resource mapping
    OutputSlotMap output_map_;           ///< Output slot name to resource mapping
};

// Simple pass that processes an input texture and outputs to another texture
class ColorProcessingPass : public RenderPass {
public:
	void Execute() override {
		// Get input texture (created externally)
		TextureHandle input = input_map_.at("Input");

		// Get output texture (created externally)
		TextureHandle output = output_map_.at("Output");

		// In a real implementation, we would:
		// 1. Bind framebuffer with output texture
		// 2. Bind input texture as sampler
		// 3. Draw fullscreen quad with processing shader

		std::cout << "Processing color from texture " << input.id
			<< " to texture " << output.id << std::endl;
	}
};

// Final output pass that presents to screen
class PresentPass : public RenderPass {
public:
	void Execute() override {
		TextureHandle input = input_map_.at("ScreenInput");

		// In a real implementation, we would:
		// 1. Bind default framebuffer
		// 2. Draw fullscreen quad with input texture
		// 3. Swap buffers

		std::cout << "Presenting texture " << input.id << " to screen" << std::endl;
	}
};

// Helper function to create an OpenGL texture
inline TextureHandle CreateColorTexture(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Create empty texture
	std::vector<float> pixels(width * height * 4, 1); // White texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_FLOAT, pixels.data());

	// Set parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return TextureHandle{ textureID };
}

NAMESPACE_END(dream)