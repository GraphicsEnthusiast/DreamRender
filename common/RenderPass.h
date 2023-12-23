#pragma once

#include <vector>

class RenderPass {
public:
	unsigned int fbo = 0;
	unsigned int vao, vbo;
	std::vector<unsigned int> colorAttachments;
	unsigned int program;
	int width = 0;
	int height = 0;

public:
	void BindData(bool finalPass = false);

	void Draw(std::vector<unsigned int> texPassArray = {});
};