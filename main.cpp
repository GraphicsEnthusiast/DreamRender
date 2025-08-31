#include <interface.h>

int main() {
	dream::ThreadPool::Instance(1);
	auto gui = dream::Interface::Create(1800, 1200);
	dream::TriangleMeshManager::Instance();
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	dream::ThreadPool::Release();
	dream::TriangleMeshManager::Release();
	return 0;
}