#pragma once

#include <iomanip>
#include <Utils.h>
#include <Shader.h>
#include <RenderPass.h>
#include <Integrator.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <OpenImageDenoise/oidn.hpp>

class Render {
public:
	Render(shared_ptr<Integrator> i, bool use_den) : inte(i), Width(inte->scene->width), Height(inte->scene->height), use_denoise(use_den) {}

	unsigned int GetTextureRGB32F(int w, int h);

	virtual bool Init() = 0;
	virtual void Run() = 0;
	virtual void Destory() = 0;

public:
	shared_ptr<Integrator> inte;
	int Width, Height;
	GLFWwindow* window;
	bool use_denoise;
};

class CPURender : public Render {
public:
	CPURender(shared_ptr<Integrator> i, bool use_den) : Render(i, use_den), frameCounter(0) {}

	virtual bool Init() override;
	virtual void Run() override;
	virtual void Destory() override;

private:
	clock_t t1, t2;
	double dt, fps;
	unsigned int frameCounter;
	RenderPass pass1;
	RenderPass pass2;
	RenderPass pass3;
	unsigned int lastFrame;
	unsigned int nowFrame;
	unsigned int denoisedFrame;
	GBuffer g;
	vec3* nowTexture;
	vec3* frameOutputPtr;
};