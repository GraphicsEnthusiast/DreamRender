#pragma once

#include <console.h>
#include <render_graph.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Interface
 * @brief Main GUI management class for the application
 */
class Interface : public std::enable_shared_from_this<Interface> {
public:
    /**
     * @brief Creates an Interface instance
     * @param width Initial window width (default: 2048)
     * @param height Initial window height (default: 1024)
     * @return Shared pointer to the interface instance
     */
    static std::shared_ptr<Interface> Create(unsigned int width = 2048, unsigned int height = 1024);

    /**
     * @brief Main rendering loop
     */
    void Render();

    /**
     * @brief Cleans up resources and terminates GLFW/ImGui contexts
     */
    ~Interface();

    /**
     * @brief Gets the main GLFW window handle
     * @return Pointer to the main GLFW window
     */
    GLFWwindow* GetMainWindow() const noexcept;

protected:
    /**
     * @brief Constructs the interface with specified dimensions
     * @param width Initial window width (default: 2048)
     * @param height Initial window height (default: 1024)
     */
    Interface(unsigned int width = 2048, unsigned int height = 1024);

    /**
     * @brief Registers SPDlog callback to redirect logs to console
     */
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

    /**
     * @brief Creates the main menu bar with dropdown options
     */
    void CreateMenuBar();

    /**
     * @brief Applies custom dark color theme to ImGui interface
     */
    void ApplyDarkTheme();

protected:
    GLFWwindow* window_;                ///< GLFW window handle
    std::unique_ptr<Console> console_;  ///< Console for log display
    unsigned int width_;                ///< Current window width
    unsigned int height_;               ///< Current window height
};

NAMESPACE_END(dream)