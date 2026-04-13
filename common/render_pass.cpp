#include <render_pass.h>
#include <srgb_to_spectrum.h>
#include <sobol_matrices_1024x52.h>
#include <cie.h>

NAMESPACE_BEGIN(dream)

std::unique_ptr<SSBO> RenderPass::srgb_to_spectrum_ssbo_ = nullptr;
std::unique_ptr<SSBO> RenderPass::sobol_matrices_ssbo_ = nullptr;
std::unique_ptr<SSBO> RenderPass::cie_ssbo_ = nullptr;
unsigned int RenderPass::frame_counter_ = 0;

void RenderPass::IncreaseFrameCounter() noexcept {
	frame_counter_++;
}

unsigned int RenderPass::GetFrameCounter() noexcept {
	return frame_counter_;
}

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
	if (srgb_to_spectrum_ssbo_) {
		return;
	}

	unsigned int total_size = 3 * 64 * 64 * 64 * 3 * sizeof(float);

	srgb_to_spectrum_ssbo_ = std::make_unique<SSBO>(
		total_size,
		SRGBToSpectrumTableData,
		GL_DYNAMIC_READ,
		GL_MAP_READ_BIT
		);

	srgb_to_spectrum_ssbo_->BindBase(2);
}

void RenderPass::InitSobolMatricesTable() {
	if (sobol_matrices_ssbo_) {
		return;
	}

	unsigned int total_size = 1024 * 52 * sizeof(unsigned int);

	sobol_matrices_ssbo_ = std::make_unique<SSBO>(
		total_size,
		SobolMatricesTableData,
		GL_DYNAMIC_READ,
		GL_MAP_READ_BIT
		);

	sobol_matrices_ssbo_->BindBase(3);
}

void RenderPass::InitCIETable() {
	if (cie_ssbo_) {
		return;
	}

	// CIE tables contain 471 samples each for X, Y, Z, and D65
	unsigned int total_size = 4 * NCIESamples * sizeof(float);

	// Prepare combined CIE data
	std::vector<float> cie_data(4 * NCIESamples);

	// Copy data in order: X, Y, Z, D65
	for (int i = 0; i < NCIESamples; ++i) {
		cie_data[i] = CIEX[i];
		cie_data[i + NCIESamples] = CIEY[i];
		cie_data[i + 2 * NCIESamples] = CIEZ[i];
		cie_data[i + 3 * NCIESamples] = D65[i];
	}

	cie_ssbo_ = std::make_unique<SSBO>(
		total_size,
		cie_data.data(),
		GL_STATIC_READ,
		GL_MAP_READ_BIT
		);

	cie_ssbo_->BindBase(8);  // Bind to binding point 8
}

SSBO& RenderPass::GetSobolMatricesSSBO() noexcept {
	return *sobol_matrices_ssbo_;
}

SSBO& RenderPass::GetSRGBToSpectrumSSBO() noexcept {
	return *srgb_to_spectrum_ssbo_;
}

SSBO& RenderPass::GetCIESSBO() noexcept {
	return *cie_ssbo_;
}

void RenderPass::BindSRGBToSpectrumSSBO(GLuint index) noexcept {
	if (srgb_to_spectrum_ssbo_) {
		srgb_to_spectrum_ssbo_->BindBase(index);
	}
}

void RenderPass::BindSobolMatricesSSBO(GLuint index) noexcept {
	if (sobol_matrices_ssbo_) {
		sobol_matrices_ssbo_->BindBase(index);
	}
}

void RenderPass::BindCIESSBO(GLuint index) noexcept {
	if (cie_ssbo_) {
		cie_ssbo_->BindBase(index);
	}
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

	shader_->Use();
	shader_->SetUInt("FrameCounter", GetFrameCounter());

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
}

PostProcessingPass::PostProcessingPass(unsigned int width, unsigned int height) {
	name_ = "Post processing pass";
	width_ = width;
	height_ = height;

	// Create compute shader for post-processing
	const char* compute_path = "../shader/post_processing.comp";
	shader_ = std::make_unique<ComputationShader>(compute_path);
}

void PostProcessingPass::Execute() {
	if (!shader_) {
		ERROR("[error] Post processing pass: Compute shader is not initialized.");

		return;
	}

	shader_->Use();

	// Retrieve input and output texture handles
	TextureHandle input_texture = GetInputTexture("Input");
	TextureHandle output_texture = GetOutputTexture("Output");

	if (!input_texture.IsValid() || !output_texture.IsValid()) {
		ERROR("[error] Post-processing pass: Required textures are invalid!");

		return;
	}

	// Bind input and output images
	glBindImageTexture(0, input_texture.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, output_texture.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Dispatch compute shader
	glDispatchCompute(width_ / 16, height_ / 16, 1);

	BufferObject::Barrier(GL_ALL_BARRIER_BITS);
}

ConvertPass::ConvertPass(unsigned int width, unsigned int height) {
	name_ = "convert pass";
	width_ = width;
	height_ = height;

	// Create the compute shader for conversion
	const char* compute_shader_path = "../shader/convert_float_to_vec3.comp";
	shader_ = std::make_unique<ComputationShader>(compute_shader_path);
}

void ConvertPass::Execute() {
	if (!shader_) {
		ERROR("[error] Convert pass: Compute shader is not initialized.");

		return;
	}

	// Use (bind) the compute shader program
	shader_->Use();

	// Retrieve and validate the texture handles
	TextureHandle input_r = GetInputTexture("InputR");
	TextureHandle input_g = GetInputTexture("InputG");
	TextureHandle input_b = GetInputTexture("InputB");
	TextureHandle output_texture = GetOutputTexture("Output");

	if (!input_r.IsValid() || !input_g.IsValid() || !input_b.IsValid() || !output_texture.IsValid()) {
		ERROR("[error] Convert pass: One or more required textures are invalid!");

		return;
	}

	// Bind input integer textures
	glBindImageTexture(0, input_r.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, input_g.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(2, input_b.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

	// Bind output float texture
	glBindImageTexture(3, output_texture.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Dispatch compute shader
	glDispatchCompute(width_ / 16, height_ / 16, 1);

	// Ensure all writes are completed
	BufferObject::Barrier(GL_ALL_BARRIER_BITS);
}

PTPass::PTPass(unsigned int width, unsigned int height) {
	name_ = "pt pass"; // Consistent naming style with other passes
	width_ = width;
	height_ = height;

	// Create the compute shader
	const char* compute_shader_path = "../shader/path_tracing.comp"; // Consider making path configurable
	shader_ = std::make_unique<ComputationShader>(compute_shader_path);
}

void PTPass::Execute() {
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

	auto& scene_manager = SceneManager::Instance();

	// Bind regular geometry buffers (existing code)
	scene_manager.GetTriangleTBO().BindTexture(0);
	shader_->SetInt("Triangles", 0);
	scene_manager.GetBVHNodeTBO().BindTexture(1);
	shader_->SetInt("BVHNodes", 1);

	// Bind utility SSBOs
	RenderPass::BindSRGBToSpectrumSSBO(2);
	shader_->SetInt("SRGBToSpectrumTable", 2);

	RenderPass::BindSobolMatricesSSBO(3);
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

	GLuint texture_array = scene_manager.GetTextureArray();
	int texture_count = scene_manager.GetTextureCount();

	if (0 != texture_array && texture_count > 0) {
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
		shader_->SetInt("TextureArray", 7);
		shader_->SetInt("TextureCount", texture_count);
	}
	else {
		shader_->SetInt("TextureArray", 7);
		shader_->SetInt("TextureCount", 0);
	}

	// Bind CIE data SSBO
	RenderPass::BindCIESSBO(8);
	shader_->SetInt("CIETable", 8);

	shader_->SetUInt("FrameCounter", GetFrameCounter());

	glDispatchCompute(width_ / 16, height_ / 16, 1);

	BufferObject::Barrier(GL_ALL_BARRIER_BITS);
}

LTPass::LTPass(unsigned int width, unsigned int height) {
	name_ = "lt pass";
	width_ = width;
	height_ = height;

	// Create the compute shader for light tracing
	const char* compute_shader_path = "../shader/light_tracing.comp";
	shader_ = std::make_unique<ComputationShader>(compute_shader_path);
}

void LTPass::Execute() {
	if (!shader_) {
		ERROR("[error] Light tracing pass: Compute shader is not initialized.");

		return;
	}

	// Use (bind) the compute shader program
	shader_->Use();

	// Retrieve and validate the output texture handle
	if (!output_texture_r_.IsValid() || !output_texture_g_.IsValid() || !output_texture_b_.IsValid()) {
		ERROR("[error] Light tracing pass: One or more output texture handles are invalid.");

		return;
	}

	float zero_f = 0.0f;
	glClearTexImage(output_texture_r_.id, 0, GL_RED, GL_FLOAT, &zero_f);
	glClearTexImage(output_texture_g_.id, 0, GL_RED, GL_FLOAT, &zero_f);
	glClearTexImage(output_texture_b_.id, 0, GL_RED, GL_FLOAT, &zero_f);

	// Bind output textures as images for atomic writes
	glBindImageTexture(0, output_texture_r_.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(1, output_texture_g_.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(2, output_texture_b_.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	auto& scene_manager = SceneManager::Instance();

	// Bind regular geometry buffers
	scene_manager.GetTriangleTBO().BindTexture(0);
	shader_->SetInt("Triangles", 0);
	scene_manager.GetBVHNodeTBO().BindTexture(1);
	shader_->SetInt("BVHNodes", 1);

	// Bind utility SSBOs
	RenderPass::BindSRGBToSpectrumSSBO(2);
	shader_->SetInt("SRGBToSpectrumTable", 2);

	RenderPass::BindSobolMatricesSSBO(3);
	shader_->SetInt("SobolMatricesTable", 3);

	// Bind light geometry buffers
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

	// Bind texture array
	GLuint texture_array = scene_manager.GetTextureArray();
	int texture_count = scene_manager.GetTextureCount();

	if (0 != texture_array && texture_count > 0) {
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
		shader_->SetInt("TextureArray", 7);
		shader_->SetInt("TextureCount", texture_count);
	}
	else {
		shader_->SetInt("TextureArray", 7);
		shader_->SetInt("TextureCount", 0);
	}

	// Bind CIE data SSBO
	RenderPass::BindCIESSBO(8);
	shader_->SetInt("CIETable", 8);

	// Set frame counter
	shader_->SetUInt("FrameCounter", GetFrameCounter());

	glDispatchCompute(width_ / 16, height_ / 16, 1);

	// Ensure all writes are completed
	BufferObject::Barrier(GL_ALL_BARRIER_BITS);
}

TextureHandle LTPass::GetOutputTextureR() const noexcept {
	return output_texture_r_;
}

TextureHandle LTPass::GetOutputTextureG() const noexcept {
	return output_texture_g_;
}

TextureHandle LTPass::GetOutputTextureB() const noexcept {
	return output_texture_b_;
}

void LTPass::SetOutputTextureR(const TextureHandle& handle) {
	output_texture_r_ = handle;
}

void LTPass::SetOutputTextureG(const TextureHandle& handle) {
	output_texture_g_ = handle;
}

void LTPass::SetOutputTextureB(const TextureHandle& handle) {
	output_texture_b_ = handle;
}


NAMESPACE_END(dream)