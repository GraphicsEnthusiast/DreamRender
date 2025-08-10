#include <interface.h>

NAMESPACE_BEGIN(dream)

Interface::Interface(unsigned int width, unsigned int height): width_(width), height_(height) {
	spdlog::set_level(spdlog::level::trace);
	// Bind console output to spdlog log callback
	RegisterLogCallback();

	// Create console
	console_ = std::make_unique<Console>();

	if (!glfwInit()) {
		ERROR("[error] GLFW initialization failed!");
		glfwTerminate();
		exit(0);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window_ = glfwCreateWindow(width_, height_, "dream", nullptr, nullptr);
	if (nullptr == window_) {
		ERROR("[error] Window creation failed!");
		glfwTerminate();
		exit(0);
	}
	glfwMakeContextCurrent(window_);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		ERROR("[error] GLAD initialization failed!");
		glfwTerminate();
		exit(0);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Enable docking
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window_, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

Interface::~Interface() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window_);
	glfwTerminate();
}

void Interface::RegisterLogCallback() {
	auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
		[this](const spdlog::details::log_msg& msg) {
			console_->AddLog(msg.payload.data());
			// for example you can be notified by sending an email to yourself
		}
	);

	callback_sink->set_level(spdlog::level::trace);

	//  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	//  spdlog::default_logger()->sinks().push_back(console_sink);
	spdlog::default_logger()->sinks().push_back(callback_sink);
}

void Interface::ConfigureAndSubmitDockspace(unsigned int display_w, unsigned int display_h) {
	ImGuiIO& io = ImGui::GetIO();
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	// Viewport resizing on window size change
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	viewport->Size = { (float)display_w, (float)display_h };
	viewport->WorkSize = { (float)display_w, (float)display_h };
	viewport->Flags |= (viewport->Flags & ImGuiViewportFlags_IsMinimized); // Preserve existing flags

	// Set dockspace window position and size based on viewport
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	// Set dockspace window style (no border, no padding, no rounding yata yata)
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	// Set dockspace window flags (menu bar, no back ground etc)
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	windowFlags |= ImGuiWindowFlags_NoNavFocus;
	windowFlags |= ImGuiWindowFlags_NoBackground;

	bool p;
	ImGui::Begin("DockSpace", &p, windowFlags);
	// Submit dockspace inside a window
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	ImGui::End();
	ImGui::PopStyleVar(3);
}

void Interface::ApplyDarkTheme() {
	ImColor text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImColor textDisabled = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

	//  // Blue scent
	//  ImColor scent       = ImVec4( 42.0f / 255.0f, 0.0f / 255.0f, 164.0f / 255.0f, 1.0f );
	//  ImColor scentActive       = ImVec4( 0.0f / 255.0f, 163.0f / 255.0f, 255.0f / 255.0f, 1.0f );
	//  ImColor secondScent = scent;

	// Warm scent
	ImColor scent = ImVec4(191.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f);
	ImColor scentActive = ImVec4(217.0f / 255.0f, 83.0f / 255.0f, 35.0f / 255.0f, 1.0f);
	ImColor secondScent = scent;

	ImColor bg = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	ImColor bgDark = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImColor hover = scentActive;

	ImColor border = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	ImColor shadow = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = text;
	colors[ImGuiCol_TextDisabled] = textDisabled;
	colors[ImGuiCol_WindowBg] = bg;
	colors[ImGuiCol_ChildBg] = bgDark;
	colors[ImGuiCol_PopupBg] = bg;
	colors[ImGuiCol_Border] = border;
	colors[ImGuiCol_BorderShadow] = shadow;
	colors[ImGuiCol_FrameBg] = bgDark;
	colors[ImGuiCol_FrameBgHovered] = hover;
	colors[ImGuiCol_FrameBgActive] = scent;
	colors[ImGuiCol_TitleBg] = bgDark;
	colors[ImGuiCol_TitleBgActive] = scent;
	colors[ImGuiCol_TitleBgCollapsed] = scent;
	colors[ImGuiCol_MenuBarBg] = bg;
	colors[ImGuiCol_ScrollbarBg] = bgDark;
	colors[ImGuiCol_ScrollbarGrab] = scent;
	colors[ImGuiCol_ScrollbarGrabHovered] = hover;
	colors[ImGuiCol_ScrollbarGrabActive] = scentActive;
	colors[ImGuiCol_CheckMark] = scent;
	colors[ImGuiCol_SliderGrab] = scent;
	colors[ImGuiCol_SliderGrabActive] = scentActive;
	colors[ImGuiCol_Button] = scent;
	colors[ImGuiCol_ButtonHovered] = hover;
	colors[ImGuiCol_ButtonActive] = scentActive;
	colors[ImGuiCol_Header] = secondScent;
	colors[ImGuiCol_HeaderHovered] = hover;
	colors[ImGuiCol_HeaderActive] = scent;
	colors[ImGuiCol_Separator] = scent;
	colors[ImGuiCol_SeparatorHovered] = hover;
	colors[ImGuiCol_SeparatorActive] = scent;
	colors[ImGuiCol_ResizeGrip] = scent;
	colors[ImGuiCol_ResizeGripHovered] = hover;
	colors[ImGuiCol_ResizeGripActive] = scentActive;
	colors[ImGuiCol_Tab] = scent;
	colors[ImGuiCol_TabHovered] = hover;
	colors[ImGuiCol_TabActive] = scent;
	colors[ImGuiCol_TabUnfocused] = secondScent;
	colors[ImGuiCol_TabUnfocusedActive] = scent;
	colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.0f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	colors[ImGuiCol_PlotLines] = scent;
	colors[ImGuiCol_PlotLinesHovered] = hover;
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	colors[ImGuiCol_PlotHistogramHovered] = hover;
	colors[ImGuiCol_TableHeaderBg] = bg;
	colors[ImGuiCol_TableBorderStrong] = border;
	colors[ImGuiCol_TableBorderLight] = shadow;
	colors[ImGuiCol_TableRowBg] = bg;
	colors[ImGuiCol_TableRowBgAlt] = bgDark;
	colors[ImGuiCol_TextSelectedBg] = hover;
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.0f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 0.0f, 0.0f, 0.7f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.2f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.35f);

	int scentRounding = 1;
	int noRounding = 0;
	int maxRounding = 5;

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.FramePadding = ImVec2(5.0f, 2.0f);
	style.CellPadding = ImVec2(6.0f, 6.0f);
	style.ItemSpacing = ImVec2(6.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 5;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 0;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.FrameBorderSize = 0;
	style.TabBorderSize = 0;
	style.WindowRounding = maxRounding;
	style.ChildRounding = maxRounding;
	style.FrameRounding = scentRounding;
	style.PopupRounding = noRounding;
	style.ScrollbarRounding = noRounding;
	style.GrabRounding = scentRounding;
	style.TabRounding = scentRounding;
	style.LogSliderDeadzone = 4;

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

void Interface::SelectableOptionFromFlag(const char* name, bool& flag) {
	static std::map<std::string, bool> selectStates;
	selectStates[name] = flag;

	ImGui::PushStyleColor(ImGuiCol_Header, { 0.0f, 0.0f, 0.0f, 0.0f });
	if (ImGui::Selectable(name, &selectStates[name])) {
		flag = selectStates[name];
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::Text("\t\t\t\t");
	ImGui::SameLine(ImGui::GetWindowWidth() - 30);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.0f, 0.0f, 0.0f, 0.0f });
	ImGui::Checkbox("##", &selectStates[name]);
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void Interface::CreateMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::MenuItem("Do nothing...");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Rendering")) {
			bool flag = true;
			SelectableOptionFromFlag("Wireframe", flag);

			if (ImGui::MenuItem("Disabled option", "[No shortcut]", false, false)) {

			} // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Do nothing", "CTRL+X")) {

			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Visualization")) {
			bool flag = true;
			SelectableOptionFromFlag("Actuator", flag);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void Interface::Render() {
	GLuint texture = 0;
	auto RenderImage = [&texture]() {
		ImGui::Begin("Rendering Window");
		ImVec2 rendering_window_size = ImGui::GetContentRegionAvail();
		static std::vector<float> pixels(rendering_window_size.x * rendering_window_size.y * 3, 1.0f);
		
		if (0 == texture) {
			glGenTextures(1, &texture);
		}
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rendering_window_size.x, rendering_window_size.y, 0, GL_RGB, GL_FLOAT, pixels.data());
		ImGui::Image((void*)(intptr_t)texture, rendering_window_size, ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();
	};

	while (!glfwWindowShouldClose(window_)) {
		glfwPollEvents();

		int display_w, display_h;
		glfwGetFramebufferSize(window_, &display_w, &display_h);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ApplyDarkTheme();  // Set theme
		// Will allow windows to be docked
		ConfigureAndSubmitDockspace(display_w, display_h);

		CreateMenuBar();
		console_->Draw();
		RenderImage();
		ImGui::ShowDemoWindow(nullptr);

		ImGui::EndFrame();
		ImGui::Render();

		glViewport(0, 0, display_w, display_h);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window_);
	}
}

NAMESPACE_END(dream)