#pragma once

#include "Utils.h"
#include "Integrator.h"
#include "Shader.h"
#include "RenderPass.h"

class Renderer {
public:
	Renderer(std::shared_ptr<Integrator> inte, const PostProcessing& p);

	~Renderer();

	void Run();

private:
	unsigned int GetTextureRGB32F(int w, int h);

private:
	std::shared_ptr<Integrator> integrator;
	PostProcessing post;
	GLFWwindow* window;
	unsigned int lastFrame, nowFrame;
	int width, height;
	clock_t t1, t2;
	double dt, fps;
	unsigned int frameCounter;
	RenderPass pass1;
	RenderPass pass2;
	RenderPass pass3;
};