#include <interface.h>

NAMESPACE_BEGIN(dream)

std::shared_ptr<Interface> Interface::Create(unsigned int width, unsigned int height) {
    auto interface = std::shared_ptr<Interface>(new Interface(width, height));

	// Associate interface with renderer
    interface->renderer_->SetInterface(interface->shared_from_this());

    return interface;
}

Interface::Interface(unsigned int width, unsigned int height) : width_(width), height_(height) {
    spdlog::set_level(spdlog::level::trace);
    RegisterLogCallback();

    console_ = std::make_unique<Console>();

    // Initialize GLFW windowing subsystem
    if (!glfwInit()) {
        ERROR("[error] GLFW initialization failed!");
        glfwTerminate();
        exit(0);
    }
    // Configure OpenGL context hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create primary application window
    window_ = glfwCreateWindow(width_, height_, "dream", nullptr, nullptr);
    if (nullptr == window_) {
        ERROR("[error] Window creation failed!");
        glfwTerminate();
        exit(0);
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // Enable vertical synchronization

    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        ERROR("[error] GLAD initialization failed!");
        glfwTerminate();
        exit(0);
    }

    // Initialize ImGui core context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Enable docking and multi-viewport features
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();  // Apply default dark theme

    // Initialize platform bindings
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 460");

	// Create renderer
	renderer_ = std::make_shared<Renderer>();
}

Interface::~Interface() {
    // Shutdown ImGui subsystems
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Release GLFW resources
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Interface::RegisterLogCallback() {
    // Create sink forwarding logs to console
    auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
        [this](const spdlog::details::log_msg& msg) {
            console_->AddLog(msg.payload.data());
        }
    );
    callback_sink->set_level(spdlog::level::trace);
    spdlog::default_logger()->sinks().push_back(callback_sink);
}

void Interface::ConfigureAndSubmitDockspace(unsigned int display_w, unsigned int display_h) {
    ImGuiIO& io = ImGui::GetIO();
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // Synchronize viewport dimensions with window size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    viewport->Size = { (float)display_w, (float)display_h };
    viewport->WorkSize = { (float)display_w, (float)display_h };
    viewport->Flags |= (viewport->Flags & ImGuiViewportFlags_IsMinimized);

    // Position dockspace to occupy entire viewport
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Configure visual styling
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // Set dock space window attributes
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar;
    windowFlags |= ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize;
    windowFlags |= ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    windowFlags |= ImGuiWindowFlags_NoNavFocus;
    windowFlags |= ImGuiWindowFlags_NoBackground;

    // Create dock space container
    bool p;
    ImGui::Begin("DockSpace", &p, windowFlags);
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    // Initialize layout on first execution
    static bool first_run = true;
    if (first_run) {
        first_run = false;

        // Rebuild docking hierarchy
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

        // Partition dock space regions
        ImGuiID top_node, bottom_node;
        bottom_node = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.37f, nullptr, &top_node);
        ImGuiID left_node, right_node;
        right_node = ImGui::DockBuilderSplitNode(top_node, ImGuiDir_Right, 0.214f, nullptr, &left_node);

        // Assign windows to dock nodes
        ImGui::DockBuilderDockWindow("Rendering Window", left_node);
        ImGui::DockBuilderDockWindow("Dear ImGui Demo", right_node);
        ImGui::DockBuilderDockWindow("Console", bottom_node);
        ImGui::DockBuilderFinish(dockspace_id);

        // Set default floating window properties
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void Interface::ApplyDarkTheme() {
    // Define color palette components
    ImColor text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImColor textDisabled = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
    ImColor scent = ImVec4(191.0f / 255.0f, 31.0f / 255.0f, 31.0f / 255.0f, 1.0f);
    ImColor scentActive = ImVec4(217.0f / 255.0f, 83.0f / 255.0f, 35.0f / 255.0f, 1.0f);
    ImColor secondScent = scent;
    ImColor bg = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    ImColor bgDark = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    ImColor hover = scentActive;
    ImColor border = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    ImColor shadow = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    // Apply color scheme to style elements
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = textDisabled;
    colors[ImGuiCol_WindowBg] = bg;
    colors[ImGuiCol_ChildBg] = bgDark;
    colors[ImGuiCol_PopupBg] = bg;

    // Configure style metrics
    int scentRounding = 1;
    int noRounding = 0;
    int maxRounding = 5;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 2.0f);

    // Adjust for multi-viewport rendering
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void Interface::SelectableOptionFromFlag(const char* name, bool& flag) {
    static std::map<std::string, bool> selectStates;
    selectStates[name] = flag;

    // Apply custom styling
    ImGui::PushStyleColor(ImGuiCol_Header, { 0.0f, 0.0f, 0.0f, 0.0f });
    if (ImGui::Selectable(name, &selectStates[name])) {
        flag = selectStates[name];
    }
    ImGui::PopStyleColor();

    // Position checkbox at right edge
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
        // File operations menu
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Do nothing...");
            ImGui::EndMenu();
        }

        // Rendering controls menu
        if (ImGui::BeginMenu("Rendering")) {
            bool flag = true;
            SelectableOptionFromFlag("Wireframe", flag);

            if (ImGui::MenuItem("Disabled option", "[No shortcut]", false, false)) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Do nothing", "CTRL+X")) {
            }
            ImGui::EndMenu();
        }

        // Visualization settings menu
        if (ImGui::BeginMenu("Visualization")) {
            bool flag = true;
            SelectableOptionFromFlag("Actuator", flag);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Interface::Render() {
	// Render output
	auto RenderOutput = [this]() {
		ImGui::Begin("Rendering Window");
		ImVec2 size = ImGui::GetContentRegionAvail();

		// Cache rendering window dimensions
		SetRenderingWindowSize(size);

		GLuint tex = renderer_->GetLatestTexture();
		if (0 != tex) {
			ImGui::Image((void*)(intptr_t)tex, size, ImVec2(0, 1), ImVec2(1, 0));
		}
		else {
			// Placeholder while rendering
			ImGui::Text("Rendering in progress...");
		}

		ImGui::End();
	};

    // Main application loop
    while (!glfwWindowShouldClose(window_)) {
		// Update frame timing
		float current_time = static_cast<float>(glfwGetTime());
		float delta_time = current_time - frame_timer_;
		frame_timer_ = current_time;

		// Request new frame every 33ms (30fps)
		if (delta_time > 0.033f) {
			RequestFrame();
		}

        glfwPollEvents();

        // Retrieve current framebuffer dimensions
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);

        // Begin new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ApplyDarkTheme();  // Apply UI theme
        ConfigureAndSubmitDockspace(display_w, display_h);  // Setup layout
        CreateMenuBar();  // Render top menu
        console_->Draw();  // Display console
        RenderOutput();  // Render main viewport
        ImGui::ShowDemoWindow(nullptr);  // Show ImGui demo

        // Finalize and render frame
        ImGui::EndFrame();
        ImGui::Render();

        // Clear framebuffer
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Handle multi-viewport rendering
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        // Swap display buffers
        glfwSwapBuffers(window_);
    }
}

void Interface::RequestFrame() {
	if (renderer_) {
		renderer_->RequestFrame();
	}
}

GLFWwindow* Interface::GetMainWindow() const noexcept {
    return window_;
}

Renderer& Interface::GetRenderer() const noexcept {
	return *renderer_;
}

ImVec2 Interface::GetRenderingWindowSize() const noexcept {
	std::lock_guard<std::mutex> lock(size_mutex_);

	return rendering_window_size_;
}

void Interface::SetRenderingWindowSize(const ImVec2& size) {
	std::lock_guard<std::mutex> lock(size_mutex_);
	rendering_window_size_ = size;
}

NAMESPACE_END(dream)