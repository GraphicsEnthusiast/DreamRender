#include <interface.h>

int main() {
	dream::ThreadPool::Instance(1);
	auto gui = dream::Interface::Create(1800, 1200);
	dream::TriangleMeshManager::Instance();
	auto mesh = dream::TriangleMesh("C:\\Users\\17199\\Desktop\\DreamRender\\cube.obj", dream::Transform());
	std::vector<dream::TriangleMesh> meshes;
	meshes.emplace_back(mesh);
	dream::TriangleMeshManager::Instance().EncodeTriangles(meshes);
	dream::TriangleMeshManager::Instance().CreateTBO();
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	dream::ThreadPool::Release();
	dream::TriangleMeshManager::Release();
	return 0;
}