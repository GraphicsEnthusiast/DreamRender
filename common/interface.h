#pragma once

#include <console.h>
#include <render_pipeline.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Interface
 * @brief Main GUI management class for the application
 */
class Interface {
public:
    /**
     * @brief Creates an Interface instance
     * @param width Initial window width (default: 2048)
     * @param height Initial window height (default: 1024)
     * @return Shared pointer to the interface instance
     */
    static std::shared_ptr<Interface> Create(unsigned int width = 2048, unsigned int height = 1024);

    /**
     * @brief Sets the active render pipeline for the application
     * @param pipeline Unique pointer to the render pipeline instance
     */
    void SetRenderPipeline(std::unique_ptr<RenderPipeline>&& pipeline);

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

    /**
     * @brief Renders the output from the render pipeline
     */
    void RenderOutput();

    /**
     * @brief Main rendering thread function
     */
    void RenderThread();

protected:
    GLFWwindow* window_;                ///< GLFW window handle
    std::unique_ptr<Console> console_;  ///< Console for log display
    unsigned int width_;                ///< Current window width
    unsigned int height_;               ///< Current window height
    std::unique_ptr<RenderPipeline> pipeline_;
    TextureHandle front_buffer_;        ///< Front buffer (accessed by UI thread)
    TextureHandle back_buffer_;         ///< Back buffer (accessed by render thread)
    std::mutex buffer_mutex_;           ///< Mutex protecting buffer swapping
    bool buffer_updated_ = false;       ///< Flag indicating back buffer update
    std::atomic<bool> rendering_active_ = false;     ///< Render thread activity status
    unsigned int frame_counter_;        ///< Frame counter
    std::thread render_thread_;         ///< Dedicated rendering thread
};

NAMESPACE_END(dream)