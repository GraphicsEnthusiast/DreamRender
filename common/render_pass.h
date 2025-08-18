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
    /**
     * @brief Constructs a RenderPass with specified enable state
     */
    RenderPass() : enabled_(true), is_final_output_(false) {}

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
    void SetInputTexture(const std::string& slot_name, TextureHandle handle);

    /**
     * @brief Sets the texture handle for a specified output slot.
     * @param slot_name Name identifier of the output slot to modify.
     * @param handle Texture handle to assign to the slot.
     */
    void SetOutputTexture(const std::string& slot_name, TextureHandle handle);

    /**
     * @brief Marks this pass as the final output producer
     * @param is_final True to mark as final output pass
     */
    void SetAsFinalOutput(bool is_final);

    /**
     * @brief Checks if this pass produces the final render output.
     * @return true If the pass is marked as the final output producer.
     * @return false If the pass does not produce final output.
     */
    bool IsFinalOutput() const noexcept;

protected:
    /// Map type for input slot name to texture handle
    using InputSlotMap = std::unordered_map<std::string, TextureHandle>;

    /// Map type for output slot name to texture handle
    using OutputSlotMap = std::unordered_map<std::string, TextureHandle>;

    bool enabled_;                       ///< Controls pass execution
    InputSlotMap input_map_;             ///< Input slot name to resource mapping
    OutputSlotMap output_map_;           ///< Output slot name to resource mapping
    bool is_final_output_;               ///< Flag indicating final output producer
};

NAMESPACE_END(dream)