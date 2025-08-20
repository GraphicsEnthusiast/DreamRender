#include <interface.h>

int main() {
	auto gui = dream::Interface::Create(1800, 1200);
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();

	return 0;
}