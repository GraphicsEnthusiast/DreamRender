#include <render_pass.h>

NAMESPACE_BEGIN(dream)

/**
 * @brief Enable/disable the render pass
 * @param enabled New enable state
 */
void RenderPass::SetEnabled(bool enabled) {
	enabled_ = enabled;
}

/**
 * @brief Check if pass is enabled
 * @return true if enabled, false otherwise
 */
bool RenderPass::IsEnabled() const noexcept {
	return enabled_;
}

/**
 * @brief Check if pass execution completed
 * @return true if execution finished, false otherwise
 */
bool RenderPass::IsCompleted() const noexcept {
	return completed_.load();  ///< Atomic load for thread-safe access
}

NAMESPACE_END(dream)