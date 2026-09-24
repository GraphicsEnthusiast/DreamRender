#include <interface.h>

int main() {
	dream::SceneManager::Instance();
	auto gui = dream::Interface::Create(1280, 720);
	dream::RenderPass::InitSRGBToSpectrumTable();
	dream::RenderPass::InitSobolMatricesTable();
	dream::RenderPass::InitCIETable();
	auto pipeline = std::make_unique<dream::LTPipeline>(gui->GetMainWindow());
	pipeline->SetRenderingSize(dream::Point2i(1280, 720));
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	dream::SceneManager::Release();

	return 0;
}