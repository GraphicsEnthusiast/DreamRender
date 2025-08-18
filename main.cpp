#include <interface.h>
#include <render_pass.h>

int main() {
	auto gui = dream::Interface::Create(1800, 1200);
	gui->Render();

	return 0;
}