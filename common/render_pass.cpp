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

/**
* @brief Declares named input slot
* @param slot_name Name of the input slot
* @param access Required access type for this input
*/
void RenderPass::DeclareInput(const std::string& slot_name, AccessType access) {
	input_slots_.emplace_back(InputSlot{ slot_name, access });
}

/**
* @brief Declares named output slot
* @param slot_name Name of the output slot
* @param is_final Marks this output as the final render result
*/
void RenderPass::DeclareOutput(const std::string& slot_name, bool is_final) {
	output_slots_.emplace_back(OutputSlot{ slot_name });
	if (is_final) {
		is_final_output_ = true;
	}
}

/**
* @brief Gets input access type by slot name
* @param slot_name Name of the input slot
* @return AccessType for the specified slot
*/
AccessType RenderPass::GetInputAccess(const std::string& slot_name) const {
	for (const auto& slot : input_slots_) {
		if (slot.name == slot_name) {
			return slot.access;
		}
	}

	return AccessType::Read;
}

NAMESPACE_END(dream)