#include "Shader.h"
#include "RenderPass.h"
#include "Spectrum.h"
#include "PostProcessing.h"
#include "Camera.h"
#include "Scene.h"
#include "Integrator.h"

using namespace std;
using namespace glm;

clock_t t1, t2;
double dt, fps;
unsigned int frameCounter = 0;
int Width = 800;
int Height = 800;

RenderPass pass1;
RenderPass pass2;
RenderPass pass3;

unsigned int GetTextureRGB32F(int w, int h) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

int main() {
#pragma region InitOpenGL
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(Width, Height, "DreamRender", NULL, NULL);
	if (window == NULL) {
		cout << "Init Window failed!" << endl;
		glfwTerminate();

		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Init glad failed!" << endl;

		return -1;
	}

	glViewport(0, 0, Width, Height);
#pragma endregion

#pragma region PipelineConfiguration
	Shader shader1("shader/VertexShader.vert", "shader/MixFrameShader.frag");
	Shader shader2("shader/VertexShader.vert", "shader/LastFrameShader.frag");
	Shader shader3("shader/VertexShader.vert", "shader/OutputShader.frag");

	pass1.program = shader1.ID;
	pass1.width = Width;
	pass1.height = Height;
	pass1.colorAttachments.push_back(GetTextureRGB32F(pass1.width, pass1.height));
	pass1.BindData();

	pass2.program = shader2.ID;
	pass2.width = Width;
	pass2.height = Height;
	unsigned int lastFrame = GetTextureRGB32F(pass2.width, pass2.height);
	pass2.colorAttachments.push_back(lastFrame);
	pass2.BindData();

	pass3.program = shader3.ID;
	pass3.width = Width;
	pass3.height = Height;
	pass3.BindData(true);

	RGBSpectrum* nowTexture = NULL;
	unsigned int nowFrame = GetTextureRGB32F(Width, Height);
#pragma endregion

	Transform tran;
	Pinhole camera(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height);
	//Thinlens camera2(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height, 2.0f);
	Sampler* sampler = new SimpleSobol(0);
	PostProcessing post(std::make_shared<ACES>());
	RTCDevice rtc_device = rtcNewDevice(NULL);
	Scene scene(rtc_device);
	//scene.AddShape(new Sphere(Point3f(0.0f, 0.0f, 3.0f), 0.5f));
	scene.AddShape(new TriangleMesh(NULL, "teapot.obj", tran));
	scene.SetCamera(std::make_shared<Pinhole>(camera));
	scene.Commit();

	VolumetricPathTracing inte(std::make_shared<Scene>(scene), sampler, std::make_shared<Gaussian>(), Width, Height, 5);

	while (!glfwWindowShouldClose(window)) {
		t2 = clock();
		dt = (double)(t2 - t1) / CLOCKS_PER_SEC;
		fps = 1.0 / dt;
		cout << "\r";
		cout << fixed << setprecision(2) << "FPS : " << fps << "    FrameCounter: " << frameCounter;
		t1 = t2;

		nowTexture = inte.RenderImage();

		glBindTexture(GL_TEXTURE_2D, nowFrame);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Width, Height, 0, GL_RGB, GL_FLOAT, nowTexture);

		glUseProgram(pass1.program);

		glUniform1ui(glGetUniformLocation(pass1.program, "frameCounter"), frameCounter++);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, nowFrame);
		glUniform1i(glGetUniformLocation(pass1.program, "nowFrame"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lastFrame);
		glUniform1i(glGetUniformLocation(pass1.program, "lastFrame"), 1);

		pass1.Draw();
		pass2.Draw(pass1.colorAttachments);
		pass3.Draw(pass2.colorAttachments);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
