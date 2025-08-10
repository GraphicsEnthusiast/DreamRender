#pragma once

#include <console.h>

NAMESPACE_BEGIN(dream)

class Interface {
public:
	Interface(unsigned int width = 2048, unsigned int height = 1024);
	~Interface();

	void Render();

protected:
	void RegisterLogCallback();
	void ConfigureAndSubmitDockspace(unsigned int display_w, unsigned int display_h);
	void SelectableOptionFromFlag(const char* name, bool& flag);
	void CreateMenuBar();
	void ApplyDarkTheme();

public:
	GLFWwindow* window_;
	std::unique_ptr<Console> console_;
	unsigned int width_;
	unsigned int height_;
};

NAMESPACE_END(dream)