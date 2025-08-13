// render_context.h
#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class RenderContext
 * @brief Manages OpenGL contexts for multi-threaded rendering
 */
class RenderContext {
public:
    /**
     * @brief Creates a shared context based on the main window
     * @param main_window The main GLFW window context
     * @param width Context width (default=1)
     * @param height Context height (default=1)
     */
    RenderContext(GLFWwindow* main_window, int width = 1, int height = 1);

    /// Destroys the OpenGL context
    ~RenderContext();

    /// Activates this context in the current thread
    void MakeCurrent();

    /// Deactivates the current context
    void Release();

    /// Gets the GLFW window handle
    GLFWwindow* GetWindow() const noexcept;

protected:
    GLFWwindow* window_;  ///< Offscreen GLFW window
};

NAMESPACE_END(dream)