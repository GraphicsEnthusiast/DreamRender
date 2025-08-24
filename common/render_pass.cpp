#include <render_pass.h>

NAMESPACE_BEGIN(dream)

void RenderPass::SetInputTexture(const std::string& slot_name, const TextureHandle& handle) {
	auto it = input_map_.find(slot_name);
	if (input_map_.end() != it) {
		it->second = handle;
	}
	else {
		input_map_.insert({ slot_name, handle });
	}
}

void RenderPass::SetOutputTexture(const std::string& slot_name, const TextureHandle& handle) {
	auto it = output_map_.find(slot_name);
	if (output_map_.end() != it) {
		it->second = handle;
	}
	else {
		output_map_.insert({ slot_name, handle });
	}
}

TextureHandle RenderPass::GetInputTexture(const std::string& slot_name) const noexcept {
	auto it = input_map_.find(slot_name);
	if (input_map_.end() != it) {
		return it->second;
	}

	return TextureHandle{};
}

TextureHandle RenderPass::GetOutputTexture(const std::string& slot_name) const noexcept {
	auto it = output_map_.find(slot_name);
	if (output_map_.end() != it) {
		return it->second;
	}

	return TextureHandle{};
}

const std::string& RenderPass::GetName() const noexcept {
	return name_;
}

NAMESPACE_END(dream)