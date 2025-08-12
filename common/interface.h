#pragma once

#include <console.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Interface
 * @brief Main GUI management class for the application
 *
 * Handles window creation, rendering pipeline, and ImGui integration.
 */
class Interface {
public:
    /**
     * @brief Constructs the interface with specified dimensions
     * @param width Initial window width (default: 2048)
     * @param height Initial window height (default: 1024)
     */
    Interface(unsigned int width = 2048, unsigned int height = 1024);

    /// Cleans up resources and terminates GLFW/ImGui contexts
    ~Interface();

    /// Main rendering loop that drives the application
    void Render();

protected:
    /// Registers SPDlog callback to redirect logs to console
    void RegisterLogCallback();

    /**
     * @brief Configures the ImGui docking layout
     * @param display_w Current display width
     * @param display_h Current display height
     */
    void ConfigureAndSubmitDockspace(unsigned int display_w, unsigned int display_h);

    /**
     * @brief Creates a selectable option with checkbox
     * @param name Display name of the option
     * @param flag Boolean reference to store selection state
     */
    void SelectableOptionFromFlag(const char* name, bool& flag);

    /// Creates the main menu bar with dropdown options
    void CreateMenuBar();

    /// Applies custom dark color theme to ImGui interface
    void ApplyDarkTheme();

protected:
    GLFWwindow* window_;          ///< GLFW window handle
    std::unique_ptr<Console> console_;  ///< Console for log display
    unsigned int width_;          ///< Current window width
    unsigned int height_;         ///< Current window height
};

NAMESPACE_END(dream)