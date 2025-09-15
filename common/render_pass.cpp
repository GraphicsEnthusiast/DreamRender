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

ProgressivePass::ProgressivePass(unsigned int width, unsigned int height) {
	name_ = "Progressive accumulation pass";
	width_ = width;
	height_ = height;
	frame_counter_ = 0;

	// Create compute shader for progressive blending
	const char* compute_path = "../shader/progressive_blend.comp";
	shader_ = std::make_unique<ComputationShader>(compute_path);
}

void ProgressivePass::Execute() {
	if (!shader_) {
		ERROR("[error] Progressive pass: Compute shader is not initialized.");

		return;
	}

	shader_->Use();
	shader_->SetUInt("FrameCounter", frame_counter_);

	TextureHandle current_frame = GetInputTexture("CurrentFrame");
	TextureHandle previous_frame = GetInputTexture("PreviousFrame");
	TextureHandle output_texture = GetOutputTexture("Output");

	if (!current_frame.IsValid() || !previous_frame.IsValid() || !output_texture.IsValid()) {
		ERROR("[error] Progressive pass: One or more required textures are invalid!");

		return;
	}

	glBindImageTexture(0, current_frame.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, previous_frame.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, output_texture.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(width_ / 16, height_ / 16, 1);

	glCopyImageSubData(output_texture.id, GL_TEXTURE_2D, 0, 0, 0, 0,
		previous_frame.id, GL_TEXTURE_2D, 0, 0, 0, 0,
		width_, height_, 1);

	BufferObject::Barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	IncrementFrameCounter();
}

void ProgressivePass::SetFrameCounter(unsigned int frame_counter) {
	frame_counter_ = frame_counter;
}

unsigned int ProgressivePass::GetFrameCounter() const noexcept {
	return frame_counter_;
}

void ProgressivePass::IncrementFrameCounter() {
	frame_counter_++;
}

void ProgressivePass::ResetFrameCounter() {
	frame_counter_ = 0;
}

SimpleComputePass::SimpleComputePass(unsigned int width, unsigned int height) {
	name_ = "Simple compute pass"; // Consistent naming style with other passes
	width_ = width;
	height_ = height;

	// Create the compute shader
	const char* compute_shader_path = "../shader/test.comp"; // Consider making path configurable
	shader_ = std::make_unique<ComputationShader>(compute_shader_path);
}

void SimpleComputePass::Execute() {
	if (!shader_) {
		ERROR("[error] Simple compute pass: Compute shader is not initialized.");

		return;
	}

	// Use (bind) the compute shader program
	shader_->Use();

	// Retrieve and validate the output texture handle
	TextureHandle output_texture = GetOutputTexture("Output");
	if (!output_texture.IsValid()) {
		ERROR("[error] Simple compute pass: Output texture handle is invalid.");

		return;
	}

	glBindImageTexture(0, output_texture.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	auto& mesh_manager = TriangleMeshManager::Instance();

	mesh_manager.GetTriangleTBO().BindTexture(0);
	shader_->SetInt("Triangles", 0);
	mesh_manager.GetBVHNodeTBO().BindTexture(1);
	shader_->SetInt("BVHNodes", 1);
	RenderPass::srgb_to_spectrum_tbo_->BindTexture(2);
	shader_->SetInt("SRGBToSpectrumTable", 2);

	glDispatchCompute(width_ / 16, height_ / 16, 1);

	BufferObject::Barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

NAMESPACE_END(dream)
