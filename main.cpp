#include <interface.h>

void CreateSingleton() {
	dream::ThreadPool::Instance();
	dream::TriangleMeshManager::Instance();
	dream::BVH::Instance();
}

void ReleaseSingleton() {
	dream::ThreadPool::Release();
	dream::TriangleMeshManager::Release();
	dream::BVH::Release();
}

int main() {
	CreateSingleton();
	auto gui = dream::Interface::Create(1280, 720);
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->SetRenderingSize(dream::Point2i(1280, 720));
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	ReleaseSingleton();
	return 0;
}