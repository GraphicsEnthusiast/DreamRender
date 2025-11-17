#include <render_pipeline.h>

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

const glm::ivec2& RenderPipeline::GetRenderingSize() const noexcept {
	return rendering_size_;
}

void RenderPipeline::SetRenderingSize(const glm::ivec2& size) {
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

TextureHandle RenderPipeline::CreateTexture(int width, int height) {
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

void TestPipeline::Init() {
	// Load and encode mesh data for rendering
	auto mesh = TriangleMesh("C:\\Users\\17199\\Desktop\\DreamRender\\teapot.obj", Transform());
	std::vector<TriangleMesh> meshes;
	meshes.emplace_back(mesh);
	mesh = TriangleMesh("C:\\Users\\17199\\Desktop\\DreamRender\\cube.obj", Transform::Translate(0.0f, -10.0f, 0.0f));
	meshes.emplace_back(mesh);

	auto mesh2 = TriangleMesh("C:\\Users\\17199\\Desktop\\DreamRender\\quad.obj", Transform::Scale(1.5f, 1.5f, 1.5f) * Transform::Translate(0.0f, 2.0f, 0.0f));
	std::vector<TriangleMesh> meshes2;
	meshes2.emplace_back(mesh2);
	mesh2 = TriangleMesh("C:\\Users\\17199\\Desktop\\DreamRender\\teapot.obj", Transform::Translate(11.0f, 0.0f, 0.0f));
	meshes2.emplace_back(mesh2);

	auto& scene_manager = SceneManager::Instance();
	scene_manager.EncodeTriangles(meshes, false);
	scene_manager.EncodeTriangles(meshes2, true);
	scene_manager.BuildBVH();          // Build acceleration structure
	scene_manager.CreateGPUBuffers();  // Upload geometry to GPU

	// Create texture resources for pipeline stages
	TextureHandle compute_output = CreateTexture(rendering_size_.x, rendering_size_.y);
	TextureHandle previous_frame = CreateTexture(rendering_size_.x, rendering_size_.y); // Persistent storage for temporal accumulation
	TextureHandle final_output = CreateTexture(rendering_size_.x, rendering_size_.y);   // Final output target

	// Create and configure compute pass (primary rendering)
	auto compute_pass = std::make_shared<SimpleComputePass>(rendering_size_.x, rendering_size_.y);
	compute_pass->SetOutputTexture("Output", compute_output);
	AddPass("Compute", compute_pass);

	// Create and configure progressive pass (temporal accumulation)
	auto progressive_pass = std::make_shared<ProgressivePass>(rendering_size_.x, rendering_size_.y);
	progressive_pass->SetInputTexture("CurrentFrame", compute_output);   // Feed from compute pass
	progressive_pass->SetInputTexture("PreviousFrame", previous_frame); // Feedback for temporal blending
	progressive_pass->SetOutputTexture("Output", final_output);          // Output to final target
	AddPass("Progressive", progressive_pass);

	// Establish data flow: Compute → Progressive
	ConnectPasses("Compute", "Output", "Progressive", "CurrentFrame");

	// Note: Temporal feedback is managed via glCopyImageSubData in ProgressivePass::Execute
	// No explicit graph connection needed for PreviousFrame

	// Designate progressive output as final result
	SetFinalOutput(final_output);
}

NAMESPACE_END(dream)