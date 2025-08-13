#pragma once

#include <shader.h>
#include <render_context.h>

NAMESPACE_BEGIN(dream)

/**
 * @struct TextureHandle
 * @brief Represents a handle to a GPU texture resource
 *
 * Uses integer-based identifier for lightweight texture referencing.
 * Provides equality comparison operator for container compatibility.
 */
struct TextureHandle {
	uint32_t id = UINT32_MAX; ///< Unique texture identifier (UINT32_MAX = invalid handle)

	/**
	 * @brief Equality comparison operator
	 * @param other TextureHandle to compare against
	 * @return true if texture IDs match
	 */
	bool operator==(const TextureHandle& other) const {
		return other.id == id;
	}
};

/**
 * @enum AccessType
 * @brief Specifies resource access patterns for render pass dependencies
 */
enum class AccessType {
	Read,      ///< Read-only resource access
	Write,     ///< Write-only resource access
	ReadWrite  ///< Read-write resource access
};

/**
 * @struct Dependency
 * @brief Defines a resource dependency between render passes
 */
struct Dependency {
	TextureHandle resource;       ///< Texture resource required by the pass
	AccessType required_access;   ///< Required access type for the resource
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
    AccessType access;        ///< Required access type for the resource
};

// Forward declaration for friendship
class RenderGraph;

/**
 * @class RenderPass
 * @brief Abstract base class for render passes with named resource slots
 */
class RenderPass {
    friend class RenderGraph;  ///< Grant RenderGraph access to internal state

public:
    /**
     * @brief Construct a new RenderPass object
     * @param enabled Initial enabled state
     */
    RenderPass(bool enabled = true) : enabled_(enabled), is_final_output_(false), 
        completed_(false), sync_(nullptr) {}
    virtual ~RenderPass() = default;

    /**
     * @brief Sets the enabled state of the render pass
     * @param enabled New enablement state(true = active, false = disabled)
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Checks if the render pass is currently enabled
     * @return true if enabled and should execute, false otherwise
     */
    bool IsEnabled() const noexcept;

    /**
     * @brief Checks if pass execution has completed
     * @return true if execution finished, false if still pending or not started
     */
    bool IsCompleted() const noexcept;

    /**
     * @brief Execute rendering commands (pure virtual)
     * @note Implemented by derived classes to perform actual rendering work
     */
    virtual void Execute() = 0;

    /**
     * @brief Declares named input slot
     * @param slot_name Name of the input slot
     * @param access Required access type for this input
     */
    void DeclareInput(const std::string& slot_name, AccessType access);

    /**
     * @brief Declares named output slot
     * @param slot_name Name of the output slot
     * @param is_final Marks this output as the final render result
     */
    void DeclareOutput(const std::string& slot_name, bool is_final = false);

    /**
     * @brief Gets input access type by slot name
     * @param slot_name Name of the input slot
     * @return AccessType for the specified slot
     */
    AccessType GetInputAccess(const std::string& slot_name) const;

    /**
     * @brief Sets the rendering context for this pass
     * @param context RenderContext to use
     */
    void SetContext(std::shared_ptr<RenderContext> context);

    /**
     * @brief Gets the last synchronization fence for this pass
     * @return GLsync object or nullptr if no fence exists
     */
    GLsync GetSync() const noexcept;

    /**
     * @brief Sets a new synchronization fence for this pass
     * @param sync GLsync object to associate with this pass
     */
    void SetSync(GLsync sync);

    /**
     * @brief Checks if pass execution has completed
     * @return true if execution finished with memory order acquire
     */
    bool IsCompleted(std::memory_order order = std::memory_order_acquire) const noexcept;

    /**
     * @brief Marks pass as completed with specified memory order
     * @param order Memory order to use
     */
    void SetCompleted(std::memory_order order = std::memory_order_release);

protected:
    /// Input slot descriptor
    struct InputSlot {
        std::string name;     ///< Slot identifier
        AccessType access;    ///< Required access type
    };

    /// Output slot descriptor
    struct OutputSlot {
        std::string name;     ///< Slot identifier
    };

    bool enabled_;                        ///< Controls whether pass executes
    std::atomic<bool> completed_;         ///< Tracks execution completion state
    std::vector<InputSlot> input_slots_;  ///< Named input slots
    std::vector<OutputSlot> output_slots_;///< Named output slots
    bool is_final_output_;                ///< Marks pass as final output producer

    /// Populated by RenderGraph during compilation
    std::vector<Dependency> inputs_;     ///< Resolved input dependencies
    std::vector<TextureHandle> outputs_; ///< Generated output resources

    std::shared_ptr<RenderContext> context_; ///< Context for this render pass
    GLsync sync_ ;                           ///< GPU synchronization fence
};

NAMESPACE_END(dream)