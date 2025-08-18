#include <render_pass.h>

NAMESPACE_BEGIN(dream)

void RenderPass::SetEnabled(bool enabled) {
	enabled_ = enabled;
}

bool RenderPass::IsEnabled() const noexcept {
	return enabled_;
}

void RenderPass::SetInputTexture(const std::string& slot_name, TextureHandle handle) {
	auto it = input_map_.find(slot_name);
	if (input_map_.end() != it) {
		it->second = handle;
	}
	else {
		input_map_.insert({ slot_name, handle });
	}
}

void RenderPass::SetOutputTexture(const std::string& slot_name, TextureHandle handle) {
	auto it = output_map_.find(slot_name);
	if (output_map_.end() != it) {
		it->second = handle;
	}
	else {
		output_map_.insert({ slot_name, handle });
	}
}

void RenderPass::SetAsFinalOutput(bool is_final) {
	is_final_output_ = is_final;
}

bool RenderPass::IsFinalOutput() const noexcept {
	return is_final_output_;
}

NAMESPACE_END(dream)