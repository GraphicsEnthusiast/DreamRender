#include <interface.h>

int main() {
	auto gui = dream::Interface::Create(1800, 1200);

	// 创建并设置渲染管线（传递主窗口用于资源共享）
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->Init(); // 在独立上下文中创建资源
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();

	return 0;
}