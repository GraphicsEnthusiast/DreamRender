#include <interface.h>

NAMESPACE_BEGIN(dream)

std::shared_ptr<Interface> Interface::Create(unsigned int width, unsigned int height) {
	std::shared_ptr<Interface> interface(new Interface(width, height));

	return interface;
}

Interface::Interface(unsigned int width, unsigned int height) : width_(width), height_(height), frame_counter_(0) {
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

	//glfwSetWindowAttrib(window_, GLFW_RESIZABLE, GLFW_FALSE);
	//glfwSetWindowAttrib(window_, GLFW_MAXIMIZED, GLFW_FALSE);

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
	//static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode |
	//	ImGuiDockNodeFlags_NoWindowMenuButton |
	//	ImGuiDockNodeFlags_NoCloseButton |
	//	ImGuiDockNodeFlags_NoResize;

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
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	window_flags |= ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;

	// Create dock space container
	bool p;
	ImGui::Begin("DockSpace", &p, window_flags);
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

	// Initialize layout on first execution
	static bool first_run = true;
	if (first_run) {
		first_run = false;

		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		ImGuiID left_node, right_node;
		right_node = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.28f, nullptr, &left_node);

		ImGuiID demo_node, console_node;
		console_node = ImGui::DockBuilderSplitNode(right_node, ImGuiDir_Down, 0.35f, nullptr, &demo_node);

		ImGui::DockBuilderDockWindow("Rendering Window", left_node);
		ImGui::DockBuilderDockWindow("Dear ImGui Demo", demo_node);
		ImGui::DockBuilderDockWindow("Console", console_node);

		ImGui::DockBuilderFinish(dockspace_id);

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

void Interface::SetRenderPipeline(std::unique_ptr<RenderPipeline>&& pipeline) {
	pipeline_ = std::move(pipeline);
	pipeline_->Compile();
}

void Interface::Render() {
	// Start rendering thread on first run
	if (pipeline_ && !rendering_active_) {
		rendering_active_.store(true, std::memory_order_release);

		// Launch rendering in thread pool
		auto& pool = ThreadPool::Instance();
		pool.Enqueue([this] {
			// Make sure we have the correct OpenGL context for this thread
			pipeline_->MakeContextCurrent();

			// Create a fence for GPU synchronization
			GLsync fence = nullptr;

			while (rendering_active_.load(std::memory_order_relaxed)) {
				// Wait for GPU if a fence exists (max one frame in flight)
				if (fence) {
					GLenum wait_result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);

					switch (wait_result) {
					case GL_ALREADY_SIGNALED:
					case GL_CONDITION_SATISFIED:
					case GL_WAIT_FAILED:
						glDeleteSync(fence);
						fence = nullptr;
						break;
					case GL_TIMEOUT_EXPIRED:
						if (!rendering_active_.load(std::memory_order_relaxed)) {
							glDeleteSync(fence);
							fence = nullptr;
							break;
						}
						continue;
					}
				}

				// Execute pipeline
				pipeline_->Execute();

				// Create a new GPU fence
				fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

				// Get output safely
				auto output = pipeline_->GetFinalOutput();

				// Update back buffer with lock
				{
					std::unique_lock<std::mutex> lock(buffer_mutex_);
					back_buffer_ = output;
					buffer_updated_ = true;
				}
			}

			// Cleanup fence on exit
			if (fence) {
				glDeleteSync(fence);
			}
		});
	}

	// Render output
	auto RenderOutput = [this]() {
		ImGui::Begin("Rendering Window");
		//ImVec2 size = ImGui::GetContentRegionAvail();
		const Point2i& rendering_size = pipeline_->GetRenderingSize();
		ImVec2 size(rendering_size.x, rendering_size.y);

		// Check if we have a new frame to display
		bool new_frame_available = false;
		{
			std::unique_lock<std::mutex> lock(buffer_mutex_);
			if (buffer_updated_) {
				// Swap front and back buffers
				std::swap(front_buffer_, back_buffer_);
				buffer_updated_ = false;
				new_frame_available = true;
			}
		}

		if (front_buffer_.IsValid()) {
			ImGui::Image((void*)(intptr_t)front_buffer_.id, size, ImVec2(0, 1), ImVec2(1, 0));

			// Display frame rate info in corner
			ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.35f);
			if (ImGui::Begin("FPS Overlay", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing)) {
				ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
				ImGui::Text("Frame: %d", frame_counter_++);
				if (new_frame_available) {
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "New Frame");
				}
			}
			ImGui::End();
		}
		else if (pipeline_) {
			ImGui::Text("Rendering in progress...");
		}
		else {
			ImGui::Text("No render pipeline set");
		}

		ImGui::End();
	};

	// Main application loop
	while (!glfwWindowShouldClose(window_)) {
		// Ensure main thread uses main context
		glfwMakeContextCurrent(window_);

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
		CreateMenuBar();   // Render top menu
		console_->Draw();  // Display console
		RenderOutput();  // Render output
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

	// Cleanup rendering thread
	rendering_active_.store(false, std::memory_order_release);
}

GLFWwindow* Interface::GetMainWindow() const noexcept {
	return window_;
}

NAMESPACE_END(dream)