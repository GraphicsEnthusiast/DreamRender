#include <render_pass.h>
#include <srgb_to_spectrum.h>

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

SimpleComputePass::SimpleComputePass(unsigned int width, unsigned int height) {
	name_ = "Simple compute pass";

	width_ = width;
	height_ = height;

	// Create compute shader
	const char* compute_path = "../shader/test.comp";

	shader_ = std::make_unique<ComputationShader>(compute_path);

	unsigned int total_size = 3 * 64 * 64 * 64 * 3 * sizeof(float);
	tbo_ = std::make_unique<TBO>(
		SRGBToSpectrumTableData,     // ����ָ��
		total_size,                  // ���ݴ�С
		GL_R32F,                  // �ڲ���ʽ (ʹ�� RGBA32F �洢������)
		GL_STATIC_DRAW               // ʹ�÷�ʽ
		);
}

NAMESPACE_END(dream)
