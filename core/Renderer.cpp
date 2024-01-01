#include "Renderer.h"

unsigned int Renderer::GetTextureRGB32F(int w, int h) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

Renderer::Renderer(std::shared_ptr<Integrator> inte, const PostProcessing& p, int w, int h) {
	width = w;
	height = h;
	integrator = inte;
	post = p;
	frameCounter = 0;

#pragma region InitOpenGL
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "DreamRender", NULL, NULL);
	if (window == NULL) {
		std::cout << "Init Window failed!" << std::endl;
		glfwTerminate();

		assert(0);
	}
	glfwMakeContextCurrent(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Init glad failed!" << std::endl;

		assert(0);
	}

	glViewport(0, 0, width, height);
#pragma endregion

#pragma region PipelineConfiguration
	Shader shader1("shader/VertexShader.vert", "shader/MixFrameShader.frag");
	Shader shader2("shader/VertexShader.vert", "shader/LastFrameShader.frag");
	Shader shader3("shader/VertexShader.vert", "shader/OutputShader.frag");

	pass1.program = shader1.ID;
	pass1.width = width;
	pass1.height = height;
	pass1.colorAttachments.push_back(GetTextureRGB32F(pass1.width, pass1.height));
	pass1.BindData();

	pass2.program = shader2.ID;
	pass2.width = width;
	pass2.height = height;
	lastFrame = GetTextureRGB32F(pass2.width, pass2.height);
	pass2.colorAttachments.push_back(lastFrame);
	pass2.BindData();

	pass3.program = shader3.ID;
	pass3.width = width;
	pass3.height = height;
	pass3.BindData(true);
#pragma endregion
}

Renderer::~Renderer() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::Run() {
	RGBSpectrum* nowTexture = NULL;
	nowFrame = GetTextureRGB32F(width, height);

	while (!glfwWindowShouldClose(window)) {
		t2 = clock();
		dt = (double)(t2 - t1) / CLOCKS_PER_SEC;
		fps = 1.0 / dt;
		std::cout << "\r";
		std::cout << std::fixed << std::setprecision(2) << "FPS : " << fps << "    FrameCounter: " << frameCounter;
		t1 = t2;

		nowTexture = integrator->RenderImage(post);

		glBindTexture(GL_TEXTURE_2D, nowFrame);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nowTexture);

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

	if (nowTexture != NULL) {
		delete[] nowTexture;
		nowTexture == NULL;
	}

	if (integrator->sampler != NULL) {
		delete integrator->sampler;
		integrator->sampler = NULL;
	}
}