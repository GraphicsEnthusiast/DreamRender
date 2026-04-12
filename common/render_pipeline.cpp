#include <render_pipeline.h>
#include <scene.h>

NAMESPACE_BEGIN(dream)

RenderPipeline::RenderPipeline(GLFWwindow* share_window) : rendering_size_(Point2i(0)) {
	graph_ = std::make_unique<RenderGraph>();

	// Create dedicated OpenGL context sharing resources with main window
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window

	render_window_ = glfwCreateWindow(16, 16, "Pipeline Context", nullptr, share_window);
	if (!render_window_) {
		ERROR("[error] Failed to create pipeline OpenGL context.");
	}
}

RenderPipeline::~RenderPipeline() {
	if (render_window_) {
		glfwDestroyWindow(render_window_);
	}
}

void RenderPipeline::Execute() {
	graph_->Execute();
}

TextureHandle RenderPipeline::GetFinalOutput() const noexcept {
	return graph_->GetFinalOutput();
}

void RenderPipeline::Compile() {
	graph_->Compile();
}

void RenderPipeline::MakeContextCurrent() {
	if (render_window_) {
		glfwMakeContextCurrent(render_window_);
	}
	else {
		ERROR("Attempted to make invalid context current.");
	}
}

const Point2i& RenderPipeline::GetRenderingSize() const noexcept {
	return rendering_size_;
}

void RenderPipeline::SetRenderingSize(const Point2i& size) {
	rendering_size_ = size;
}

void RenderPipeline::AddPass(const std::string& name, std::shared_ptr<RenderPass> pass) {
	graph_->AddPass(name, std::move(pass));
}

void RenderPipeline::ConnectPasses(const std::string& src_pass, const std::string& src_output,
	const std::string& dst_pass, const std::string& dst_input) {
	graph_->AddEdge({ src_pass, src_output, dst_pass, dst_input });
}

void RenderPipeline::SetFinalOutput(const TextureHandle& output) {
	graph_->SetFinalOutput(output);
}

TextureHandle RenderPipeline::CreateTextureRGBA32F(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0,
		GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return TextureHandle{ textureID };
}

TextureHandle RenderPipeline::CreateTextureR32UI(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0,
		GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return TextureHandle{ textureID };
}

void PTPipeline::Init() {
	auto& scene_manager = SceneManager::Instance();

	const std::string cube_diffuse_path = "C:\\Users\\17199\\Desktop\\DreamRender\\rustediron2_basecolor.png";
	int cube_diffuse_id = scene_manager.LoadTexture(cube_diffuse_path, TextureType::DIFFUSE);

	Material teapot_material;
	teapot_material.type = MaterialType::DIFFUSE;
	teapot_material.diffuse = Vector3f(0.8f, 0.7f, 0.6f);
	teapot_material.roughness = 0.5f;
	teapot_material.emission = Vector3f(0.0f, 0.0f, 0.0f);

	Material cube_material;
	cube_material.type = MaterialType::BOUNDARY;
	cube_material.diffuse = Vector3f(0.7f, 0.7f, 0.9f);
	cube_material.roughness = 0.3f;
	cube_material.emission = Vector3f(0.0f, 0.0f, 0.0f);

	Material light_material;
	light_material.type = MaterialType::DIFFUSE;
	light_material.diffuse = Vector3f(0.9f);
	light_material.roughness = 0.0f;
	light_material.emission = Vector3f(1.5f, 1.5f, 1.0f);

	std::vector<TriangleMesh> meshes;

	meshes.emplace_back(
		"C:\\Users\\17199\\Desktop\\DreamRender\\teapot.obj",
		Transform(),
		std::make_unique<Material>(teapot_material)
	);

	meshes.emplace_back(
		"C:\\Users\\17199\\Desktop\\DreamRender\\cube.obj",
		Transform::Translate(0.0f, -10.0f, 0.0f),
		std::make_unique<Material>(cube_material),
		std::make_unique<Medium>(Medium())
	);

	std::vector<TriangleMesh> meshes2;
	meshes2.emplace_back(
		"C:\\Users\\17199\\Desktop\\DreamRender\\quad.obj",
		Transform::Scale(5.5f, 5.5f, 5.5f) * Transform::Translate(0.0f, 2.0f, 0.0f),
		std::make_unique<Material>(light_material)
	);

	scene_manager.EncodeTriangles(meshes, false);
	scene_manager.EncodeTriangles(meshes2, true);
	scene_manager.BuildBVH();
	scene_manager.CreateGPUBuffers();

	TextureHandle compute_output = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);
	TextureHandle previous_frame = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);
	TextureHandle progressive_output = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);
	TextureHandle postprocess_output = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);

	auto compute_pass = std::make_shared<PTPass>(rendering_size_.x, rendering_size_.y);
	compute_pass->SetOutputTexture("Output", compute_output);
	AddPass("Compute", compute_pass);

	auto progressive_pass = std::make_shared<ProgressivePass>(rendering_size_.x, rendering_size_.y);
	progressive_pass->SetInputTexture("CurrentFrame", compute_output);
	progressive_pass->SetInputTexture("PreviousFrame", previous_frame);
	progressive_pass->SetOutputTexture("Output", progressive_output);
	AddPass("Progressive", progressive_pass);

	auto postprocess_pass = std::make_shared<PostProcessingPass>(rendering_size_.x, rendering_size_.y);
	postprocess_pass->SetInputTexture("Input", progressive_output);
	postprocess_pass->SetOutputTexture("Output", postprocess_output);
	AddPass("PostProcessing", postprocess_pass);

	ConnectPasses("Compute", "Output", "Progressive", "CurrentFrame");
	ConnectPasses("Progressive", "Output", "PostProcessing", "Input");

	SetFinalOutput(postprocess_output);
}

void LTPipeline::Init() {
	auto& scene_manager = SceneManager::Instance();

	// Load materials and textures
	const std::string cube_diffuse_path = "C:\\Users\\17199\\Desktop\\DreamRender\\rustediron2_basecolor.png";
	int cube_diffuse_id = scene_manager.LoadTexture(cube_diffuse_path, TextureType::DIFFUSE);

	Material teapot_material;
	teapot_material.type = MaterialType::DIFFUSE;
	teapot_material.diffuse = Vector3f(0.8f, 0.7f, 0.6f);
	teapot_material.roughness = 0.5f;
	teapot_material.emission = Vector3f(0.0f, 0.0f, 0.0f);

	Material cube_material;
	cube_material.type = MaterialType::BOUNDARY;
	cube_material.diffuse = Vector3f(0.7f, 0.7f, 0.9f);
	cube_material.roughness = 0.3f;
	cube_material.emission = Vector3f(0.0f, 0.0f, 0.0f);

	Material light_material;
	light_material.type = MaterialType::DIFFUSE;
	light_material.diffuse = Vector3f(0.9f);
	light_material.roughness = 0.0f;
	light_material.emission = Vector3f(1.5f, 1.5f, 1.0f);

	std::vector<TriangleMesh> meshes;
	meshes.emplace_back(
		"C:\\Users\\17199\\Desktop\\DreamRender\\teapot.obj",
		Transform(),
		std::make_unique<Material>(teapot_material)
	);

	meshes.emplace_back(
		"C:\\Users\\17199\\Desktop\\DreamRender\\cube.obj",
		Transform::Translate(0.0f, -10.0f, 0.0f),
		std::make_unique<Material>(cube_material),
		std::make_unique<Medium>(Medium())
	);

	std::vector<TriangleMesh> meshes2;
	meshes2.emplace_back(
		"C:\\Users\\17199\\Desktop\\DreamRender\\quad.obj",
		Transform::Scale(5.5f, 5.5f, 5.5f) * Transform::Translate(0.0f, 2.0f, 0.0f),
		std::make_unique<Material>(light_material)
	);

	scene_manager.EncodeTriangles(meshes, false);
	scene_manager.EncodeTriangles(meshes2, true);
	scene_manager.BuildBVH();
	scene_manager.CreateGPUBuffers();

	// Create textures for the pipeline
	// Integer textures for atomic accumulation
	TextureHandle int_texture_r = CreateTextureR32UI(rendering_size_.x, rendering_size_.y);
	TextureHandle int_texture_g = CreateTextureR32UI(rendering_size_.x, rendering_size_.y);
	TextureHandle int_texture_b = CreateTextureR32UI(rendering_size_.x, rendering_size_.y);

	// Float textures for conversion and progressive accumulation
	TextureHandle convert_output = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);
	TextureHandle previous_frame = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);
	TextureHandle progressive_output = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);
	TextureHandle postprocess_output = CreateTextureRGBA32F(rendering_size_.x, rendering_size_.y);

	// Create light tracing integer pass
	auto lt_pass_int = std::make_shared<LTPass>(rendering_size_.x, rendering_size_.y);
	lt_pass_int->SetOutputTextureR(int_texture_r);
	lt_pass_int->SetOutputTextureG(int_texture_g);
	lt_pass_int->SetOutputTextureB(int_texture_b);
	AddPass("LightTracingInt", lt_pass_int);

	// Create integer to float conversion pass
	auto convert_pass = std::make_shared<ConvertPass>(rendering_size_.x, rendering_size_.y);
	convert_pass->SetInputTexture("InputR", int_texture_r);
	convert_pass->SetInputTexture("InputG", int_texture_g);
	convert_pass->SetInputTexture("InputB", int_texture_b);
	convert_pass->SetOutputTexture("Output", convert_output);
	AddPass("Convert", convert_pass);

	// Create progressive accumulation pass
	auto progressive_pass = std::make_shared<ProgressivePass>(rendering_size_.x, rendering_size_.y);
	progressive_pass->SetInputTexture("CurrentFrame", convert_output);
	progressive_pass->SetInputTexture("PreviousFrame", previous_frame);
	progressive_pass->SetOutputTexture("Output", progressive_output);
	AddPass("Progressive", progressive_pass);

	// Create post-processing pass
	auto postprocess_pass = std::make_shared<PostProcessingPass>(rendering_size_.x, rendering_size_.y);
	postprocess_pass->SetInputTexture("Input", progressive_output);
	postprocess_pass->SetOutputTexture("Output", postprocess_output);
	AddPass("PostProcessing", postprocess_pass);

	// Connect the passes
	ConnectPasses("LightTracingInt", "", "Convert", "");
	ConnectPasses("Convert", "Output", "Progressive", "CurrentFrame");
	ConnectPasses("Progressive", "Output", "PostProcessing", "Input");

	// Set the final output
	SetFinalOutput(postprocess_output);
}

NAMESPACE_END(dream)