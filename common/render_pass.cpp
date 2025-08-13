#include <render_pass.h>

NAMESPACE_BEGIN(dream)

void RenderPass::SetEnabled(bool enabled) {
	enabled_ = enabled;
	// Reset completion state when enabling
	if (enabled) {
		completed_.store(false, std::memory_order_release);
	}
}

bool RenderPass::IsEnabled() const noexcept {
	return enabled_;
}

bool RenderPass::IsCompleted() const noexcept {
	return completed_.load();
}

void RenderPass::DeclareInput(const std::string& slot_name, AccessType access) {
	input_slots_.emplace_back(InputSlot{ slot_name, access });
}

void RenderPass::DeclareOutput(const std::string& slot_name, bool is_final) {
	output_slots_.emplace_back(OutputSlot{ slot_name });
	if (is_final) {
		is_final_output_ = true;
	}
}

AccessType RenderPass::GetInputAccess(const std::string& slot_name) const {
	for (const auto& slot : input_slots_) {
		if (slot.name == slot_name) {
			return slot.access;
		}
	}

	return AccessType::Read;
}

void RenderPass::SetContext(std::shared_ptr<RenderContext> context) {
	context_ = context;
}

GLsync RenderPass::GetSync() const noexcept {
	return sync_;
}

void RenderPass::SetSync(GLsync sync) {
	sync_ = sync;
}

bool RenderPass::IsCompleted(std::memory_order order) const noexcept {
	return completed_.load(order);
}

void RenderPass::SetCompleted(std::memory_order order) {
	completed_.store(true, order);
}

NAMESPACE_END(dream)