#include <render_pipeline.h>

NAMESPACE_BEGIN(dream)

RenderPipeline::RenderPipeline(GLFWwindow* share_window) : rendering_size_(Point2i(0)) {
	graph_ = std::make_unique<RenderGraph>();

	// Create dedicated OpenGL context sharing resources with main window
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window
	render_window_ = glfwCreateWindow(1, 1, "Pipeline Context", nullptr, share_window);

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

NAMESPACE_END(dream)