#include <interface.h>

void CreateSingleton() {
	dream::ThreadPool::Instance(1);
	dream::TriangleMeshManager::Instance();
}

void ReleaseSingleton() {
	dream::ThreadPool::Release();
	dream::TriangleMeshManager::Release();
}

int main() {
	CreateSingleton();
	auto gui = dream::Interface::Create(1280, 720);
	dream::TriangleMeshManager::Instance();
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	ReleaseSingleton();
	return 0;
}