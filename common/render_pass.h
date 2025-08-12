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

// Forward declaration for friendship
class RenderGraph;

/**
 * @class RenderPass
 * @brief Abstract base class for render graph operations
 *
 * Encapsulates executable rendering tasks with:
 * - Dependency tracking
 * - Execution state management
 * - Enable/disable control
 *
 * @note Actual rendering logic implemented in Execute() by derived classes
 */
class RenderPass {
	friend RenderGraph;  ///< Grant RenderGraph access to internal state

public:
	/**
	 * @brief Construct a new RenderPass object
	 * @param enabled Initial enabled state (default=true)
	 */
	RenderPass(bool enabled = true) : enabled_(enabled), is_final_output_(false), completed_(false) {}

	/// Virtual destructor for polymorphic deletion
	virtual ~RenderPass() = default;

	/// @name State Management
	/// @{
	void SetEnabled(bool enabled);      ///< Set enable/disable state
	bool IsEnabled() const noexcept;    ///< Check if pass is enabled
	bool IsCompleted() const noexcept;  ///< Check if pass execution completed
	/// @}

	/**
	 * @brief Execute rendering commands (pure virtual)
	 *
	 * Implemented by derived classes to perform actual rendering work.
	 */
	virtual void Execute() = 0;

protected:
	bool enabled_;                       ///< Controls whether pass executes
	std::atomic<bool> completed_;        ///< Tracks execution completion state
	std::vector<Dependency> inputs_;     ///< Required input resources
	std::vector<TextureHandle> outputs_; ///< Generated output resources
	bool is_final_output_;               ///< Marks pass as final output producer
};

NAMESPACE_END(dream)