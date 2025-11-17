#include <render_pass.h>
#include <srgb_to_spectrum.h>
#include <sobol_matrices_1024x52.h>

NAMESPACE_BEGIN(dream)

std::unique_ptr<TBO> RenderPass::srgb_to_spectrum_tbo_ = nullptr;
std::unique_ptr<TBO> RenderPass::sobol_matrices_tbo_ = nullptr;

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

void RenderPass::InitSobolMatricesTable() {
	if (sobol_matrices_tbo_) {
		return;
	}

	unsigned int total_size = 1024 * 52 * sizeof(unsigned int);
	sobol_matrices_tbo_ = std::make_unique<TBO>(
		SobolMatricesTableData,
		total_size,
		GL_R32UI,
		GL_STATIC_DRAW
	);
}

const TBO& RenderPass::GetSobolMatricesTBO() noexcept {
    return *sobol_matrices_tbo_;
}

const TBO& RenderPass::GetSRGBToSpectrumTBO() noexcept {
    return *srgb_to_spectrum_tbo_;
}

ProgressivePass::ProgressivePass(unsigned int width, unsigned int height) {
	name_ = "Progressive accumulation pass";
	width_ = width;
	height_ = height;

	// Create compute shader for progressive blending
	const char* compute_path = "../shader/progressive_blend.comp";
	shader_ = std::make_unique<ComputationShader>(compute_path);
}

void ProgressivePass::Execute() {
	if (!shader_) {
		ERROR("[error] Progressive pass: Compute shader is not initialized.");

		return;
	}

	static unsigned int frame_counter = 0;

	shader_->Use();
	shader_->SetUInt("FrameCounter", frame_counter);

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

	BufferObject::Barrier(GL_ALL_BARRIER_BITS);

	glCopyImageSubData(output_texture.id, GL_TEXTURE_2D, 0, 0, 0, 0,
		previous_frame.id, GL_TEXTURE_2D, 0, 0, 0, 0,
		width_, height_, 1);

	BufferObject::Barrier(GL_ALL_BARRIER_BITS);

	frame_counter++;
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

	static unsigned int frame_counter = 0;

	// Use (bind) the compute shader program
	shader_->Use();

	// Retrieve and validate the output texture handle
	TextureHandle output_texture = GetOutputTexture("Output");
	if (!output_texture.IsValid()) {
		ERROR("[error] Simple compute pass: Output texture handle is invalid.");
		return;
	}

	glBindImageTexture(0, output_texture.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	auto& scene_manager = SceneManager::Instance();

	// Bind regular geometry buffers (existing code)
	scene_manager.GetTriangleTBO().BindTexture(0);
	shader_->SetInt("Triangles", 0);
	scene_manager.GetBVHNodeTBO().BindTexture(1);
	shader_->SetInt("BVHNodes", 1);

	// Bind utility buffers (existing code)
	RenderPass::GetSRGBToSpectrumTBO().BindTexture(2);
	shader_->SetInt("SRGBToSpectrumTable", 2);
	RenderPass::GetSobolMatricesTBO().BindTexture(3);
	shader_->SetInt("SobolMatricesTable", 3);

	// Bind light geometry buffers (existing code)
	scene_manager.GetTriangleLightTBO().BindTexture(4);
	shader_->SetInt("TrianglesLight", 4);
	scene_manager.GetBVHNodeLightTBO().BindTexture(5);
	shader_->SetInt("BVHNodesLight", 5);

	// Bind mesh light alias table texture buffer
	scene_manager.GetMeshLightAliasTableTBO().BindTexture(6);
	shader_->SetInt("MeshLightTable", 6);
	shader_->SetFloat("MeshLightTableMax", scene_manager.GetMeshLightTableMax());
	shader_->SetFloat("MeshLightTableSum", scene_manager.GetMeshLightTableSum());
	shader_->SetInt("MeshLightTableSize", scene_manager.GetMeshLightTableSize());

	shader_->SetUInt("FrameCounter", frame_counter);

	glDispatchCompute(width_ / 16, height_ / 16, 1);

	BufferObject::Barrier(GL_ALL_BARRIER_BITS);

	frame_counter++;
}

NAMESPACE_END(dream)
