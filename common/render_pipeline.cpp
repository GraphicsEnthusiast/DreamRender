#include <render_pipeline.h>

NAMESPACE_BEGIN(dream)

RenderPipeline::RenderPipeline() {
	graph_ = std::make_unique<RenderGraph>();
	Setup();
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