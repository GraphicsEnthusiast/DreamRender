#include <render_pipeline.h>

NAMESPACE_BEGIN(dream)

RenderPipeline::RenderPipeline(GLFWwindow* share_window) {
	graph_ = std::make_unique<RenderGraph>();

	// Create dedicated OpenGL context sharing resources with main window
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window
	render_context_ = glfwCreateWindow(1, 1, "Pipeline Context", nullptr, share_window);

	if (!render_context_) {
		ERROR("Failed to create pipeline OpenGL context.");
	}
}

RenderPipeline::~RenderPipeline() {
	if (render_context_) {
		glfwDestroyWindow(render_context_);
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
	if (render_context_) {
		glfwMakeContextCurrent(render_context_);
	}
	else {
		ERROR("Attempted to make invalid context current.");
	}
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

NAMESPACE_END(dream)