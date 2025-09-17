#include <interface.h>

void CreateSingleton() {
	dream::SceneManager::Instance();
}

void ReleaseSingleton() {
	dream::SceneManager::Release();
}

int main() {
	CreateSingleton();
	auto gui = dream::Interface::Create(1280, 720);
	dream::RenderPass::InitSRGBToSpectrumTable();
	dream::RenderPass::InitSobolMatricesTable();
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->SetRenderingSize(dream::Point2i(1280, 720));
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	ReleaseSingleton();

	return 0;
}