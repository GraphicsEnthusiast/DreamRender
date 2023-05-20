#include <Render.h>

unsigned int Render::GetTextureRGB32F(int w, int h) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

bool CPURender::Init() {
#pragma region InitOpenGL
	glfwInit();//初始化GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);//OpenGL主版本号为3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);//OpenGL次版本号为3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);//告诉GLFW我们使用的是核心模式

	window = glfwCreateWindow(Width, Height, "DreamRender", NULL, NULL);//创建窗口
	if (window == NULL) {
		cout << "Init window failed!" << endl;
		glfwTerminate();

		return false;
	}
	glfwMakeContextCurrent(window);//将window窗口的上下文设置为当前线程的主上下文

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	//glfwSetCursorPosCallback(window, mouse_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Init GLAD failed!" << endl;

		return false;
	}

	glViewport(0, 0, Width, Height);//设置渲染视口的大小

	//imgui
	const char* glsl_version = "#version 130";
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
#pragma endregion

#pragma region PipelineConfiguration
	Shader shader1("../shader/vertex.vert", "../shader/fragment.frag");
	Shader shader2("../shader/vertex.vert", "../shader/lastframe.frag");
	Shader shader3("../shader/vertex.vert", "../shader/post.frag");

	pass1.program = shader1.ID;
	pass1.width = Width;
	pass1.height = Height;
	pass1.colorAttachments.push_back(GetTextureRGB32F(pass1.width, pass1.height));
	pass1.BindData();

	pass2.program = shader2.ID;
	pass2.width = Width;
	pass2.height = Height;
	lastFrame = GetTextureRGB32F(pass2.width, pass2.height);
	pass2.colorAttachments.push_back(lastFrame);
	pass2.BindData();

	pass3.program = shader3.ID;
	pass3.width = Width;
	pass3.height = Height;
	pass3.BindData(true);

	return true;
#pragma endregion
}

void CPURender::Run() {
	nowTexture = new vec3[Width * Height];
	nowFrame = GetTextureRGB32F(Width, Height);
	denoisedFrame = GetTextureRGB32F(Width, Height);
	if (use_denoise) {
		g = inte->GetSceneGBuffer();
	}
	frameOutputPtr = new vec3[Width * Height];

	while (!glfwWindowShouldClose(window)) {
		//imgui
		//Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Render Information");
		ImGui::SameLine();
		ImGui::Text("FrameCounter = %d", frameCounter);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		//帧计时
// 		t2 = clock();
// 		dt = (double)(t2 - t1) / CLOCKS_PER_SEC;
// 		fps = 1.0 / dt;
// 		cout << "\r";
// 		cout << fixed << setprecision(5) << "FPS : " << fps << "    Iterations: " << frameCounter;
//		t1 = t2;

		omp_set_num_threads(32);//线程个数
#pragma omp parallel for
		for (int j = 0; j < Height; j++) {
			for (int i = 0; i < Width; i++) {
				vec2 jitter = inte->scene->filter->FilterVec2(vec2(RandomFloat(), RandomFloat()));

				const float px = (static_cast<float>(i) + jitter.x) / static_cast<float>(Width);
				const float py = (static_cast<float>(j) + jitter.y) / static_cast<float>(Height);

				RTCRay ray = inte->scene->camera->GenerateRay(px, py);
				RTCRayHit rayhit = MakeRayHit(ray);
				IntersectionInfo info;
				info.frameCounter = frameCounter;
				info.pixel = vec2(i, j);
				info.pixel_ndc = vec2(px, py);

				vec3 radiance = inte->GetPixelColor(info);

				if (IsNan(radiance)) {
					cout << "NAN" << endl;
					radiance = vec3(0.0f);
				}
				//color = glm::clamp(color, vec3(0.0f), vec3(5.0f));

				//这一步可以极大的减少白噪点（特别是由点光源产生）
				if (!use_denoise) {
					int lightNum = inte->scene->lights.size();
					if (inte->scene->useEnv) {
						lightNum++;
					}
					float illum = dot(radiance, vec3(0.2126f, 0.7152f, 0.072f));
					if (illum > lightNum) {
						radiance *= lightNum / illum;
					}
				}

				nowTexture[j * Width + i] = radiance;
			}
		}

		if (use_denoise) {
			//Create an Intel Open Image Denoise device
			oidn::DeviceRef device = oidn::newDevice();
			device.commit();

			//Create a denoising filter
			oidn::FilterRef filter = device.newFilter("RT"); //generic ray tracing filter
			filter.setImage("color", nowTexture, oidn::Format::Float3, Width, Height);
			filter.setImage("albedo", g.albedoTexture, oidn::Format::Float3, Width, Height);
			filter.setImage("normal", g.normalTexture, oidn::Format::Float3, Width, Height);
			filter.setImage("output", frameOutputPtr, oidn::Format::Float3, Width, Height);
			filter.set("hdr", true); //image is HDR
			filter.commit();

			//Filter the image
			filter.execute();

			//Check for errors
			const char* errorMessage;
			if (device.getError(errorMessage) != oidn::Error::None) {
				cout << "Error: " << errorMessage << endl;
			}

			glBindTexture(GL_TEXTURE_2D, denoisedFrame);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Width, Height, 0, GL_RGB, GL_FLOAT, frameOutputPtr);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, nowFrame);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Width, Height, 0, GL_RGB, GL_FLOAT, nowTexture);
		}

		glUseProgram(pass1.program);

		glUniform1ui(glGetUniformLocation(pass1.program, "frameCounter"), frameCounter++);

		glActiveTexture(GL_TEXTURE0);
		if (use_denoise) {
			glBindTexture(GL_TEXTURE_2D, denoisedFrame);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, nowFrame);
		}
		glUniform1i(glGetUniformLocation(pass1.program, "nowFrame"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lastFrame);
		glUniform1i(glGetUniformLocation(pass1.program, "lastFrame"), 1);

		//绘制
		pass1.Draw();
		pass2.Draw(pass1.colorAttachments);
		pass3.Draw(pass2.colorAttachments);

		//imgui
		//Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();//检查有没有触发什么事件（比如键盘输入、鼠标移动等）、更新窗口状态，并调用对应的回调函数（可以通过回调方法手动设置）
	}
}

void CPURender::Destory() {
	delete[] nowTexture;
	delete[] frameOutputPtr;
	if (use_denoise) {
		delete[] g.albedoTexture;
		delete[] g.normalTexture;
	}

	//Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
}