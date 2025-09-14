#include <render_pass.h>
#include <srgb_to_spectrum.h>

NAMESPACE_BEGIN(dream)

std::unique_ptr<TBO> RenderPass::srgb_to_spectrum_tbo_ = nullptr;

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

void RenderPass::InitSRGBToSpectrumTable() {
	if (srgb_to_spectrum_tbo_) {
		return;
	}

	unsigned int total_size = 3 * 64 * 64 * 64 * 3 * sizeof(float);
	srgb_to_spectrum_tbo_ = std::make_unique<TBO>(
		SRGBToSpectrumTableData,
		total_size,
		GL_R32F,
		GL_STATIC_DRAW
	);
}

SimpleComputePass::SimpleComputePass(unsigned int width, unsigned int height) {
	name_ = "Simple compute pass";

	width_ = width;
	height_ = height;

	// Create compute shader
	const char* compute_path = "../shader/test.comp";

	shader_ = std::make_unique<ComputationShader>(compute_path);
}

void SimpleComputePass::Execute() {
	// Use compute shader
	shader_->Use();

	// Bind output texture
	glBindImageTexture(0, GetOutputTexture("Output").id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	auto& mesh_manager = TriangleMeshManager::Instance();

	mesh_manager.GetTriangleTBO().BindTexture(0);
	shader_->SetInt("Triangles", 0);

	mesh_manager.GetBVHNodeTBO().BindTexture(1);
	shader_->SetInt("BVHNodes", 1);

	srgb_to_spectrum_tbo_->BindTexture(2);
	shader_->SetInt("SRGBToSpectrumTable", 2);

	// Dispatch compute shader
	glDispatchCompute(width_ / 16, height_ / 16, 1);

	BufferObject::Barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

NAMESPACE_END(dream)
