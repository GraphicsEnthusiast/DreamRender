#include "Shader.h"
#include "RenderPass.h"
#include "Spectrum.h"
#include "PostProcessing.h"
#include "Camera.h"

using namespace std;
using namespace glm;

clock_t t1, t2;
double dt, fps;
unsigned int frameCounter = 0;
int Width = 1280;
int Height = 720;

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

	RGBSpectrum* nowTexture = new RGBSpectrum[Width * Height];
	unsigned int nowFrame = GetTextureRGB32F(Width, Height);
#pragma endregion

	Pinhole camera(Matrix4f(1.0f), Width, Height, 60.0f);
	PostProcessing post(std::make_shared<ACES>());

	while (!glfwWindowShouldClose(window)) {
		t2 = clock();
		dt = (double)(t2 - t1) / CLOCKS_PER_SEC;
		fps = 1.0 / dt;
		cout << "\r";
		cout << fixed << setprecision(2) << "FPS : " << fps << "    FrameCounter: " << frameCounter;
		t1 = t2;

		omp_set_num_threads(32);
#pragma omp parallel for
		for (int j = 0; j < Height; j++) {
			for (int i = 0; i < Width; i++) {
				Ray ray = camera.GenerateRay(i, j);
				float t = 0.5f * (ray.GetDir().y + 1.0f);
				float a[3] = { 0.5f, 0.7f, 1.0f };
				float b[3] = { 1.0f, 1.0f, 1.0f };
				RGBSpectrum color1 = RGBSpectrum::FromRGB(b);
				RGBSpectrum color2 = RGBSpectrum::FromRGB(a);
				RGBSpectrum color = Lerp(t, color1, color2);
				nowTexture[j * Width + i] = color;
				//nowTexture[j * Width + i] = post.GetScreenColor(color);
			}
		}

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

	delete[] nowTexture;

	glfwTerminate();

	return 0;
}
