// render_context.cpp
#include <render_context.h>

NAMESPACE_BEGIN(dream)

RenderContext::RenderContext(GLFWwindow* main_window, int width, int height) {
    // Save current context settings
    GLFWwindow* prev_context = glfwGetCurrentContext();

    // Create shared offscreen context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ = glfwCreateWindow(width, height, "Offscreen", nullptr, main_window);

    if (!window_) {
        ERROR("[error] Failed to create offscreen context");
        return;
    }

    // Initialize GLAD for this context
    glfwMakeContextCurrent(window_);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        ERROR("[error] Failed to initialize GLAD for offscreen context");
    }

    // Restore previous context
    glfwMakeContextCurrent(prev_context);
}

RenderContext::~RenderContext() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
}

void RenderContext::MakeCurrent() {
    glfwMakeContextCurrent(window_);
}

void RenderContext::Release() {
    glfwMakeContextCurrent(nullptr);
}

GLFWwindow* RenderContext::GetWindow() const noexcept {
    return window_;
}

bool RenderContext::IsSharingWith(const RenderContext* other) const noexcept {
    if (!window_ || !other || !other->GetWindow()) {
        return false;
    }

    return glfwGetWindowUserPointer(window_) == glfwGetWindowUserPointer(other->GetWindow());
}

bool RenderContext::IsSharingWith(GLFWwindow* window) const noexcept {
    if (!window_ || !window) {
        return false;
    }

    return glfwGetWindowUserPointer(window_) == glfwGetWindowUserPointer(window);
}

NAMESPACE_END(dream)