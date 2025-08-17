#pragma once

#include <utils.h>
#include <render_graph.h>
#include <thread_pool.h>

NAMESPACE_BEGIN(dream)

// Forward declaration
class Interface;

/**
 * @class Renderer
 * @brief Manages asynchronous rendering using triple-buffered textures and fences
 */
class Renderer {
public:
    static constexpr unsigned int BUFFER_COUNT = 3;  ///< Triple buffering for latency hiding

    /// Frame buffer descriptor
    struct FrameBuffer {
        GLuint texture = 0;             ///< Texture handle
        GLsync fence = nullptr;          ///< GPU fence for synchronization
        std::atomic<bool> ready = false; ///< CPU-side ready flag
    };

    /**
     * @brief Constructs a renderer
     */
    Renderer();

    /**
     * @brief Destructor releases all resources
     */
    ~Renderer();

    /**
     * @brief Sets the render graph to execute
     * @param graph Render graph to use
     */
    void SetRenderGraph(std::unique_ptr<RenderGraph> graph);

    /**
     * @brief Gets the latest ready texture for display
     * @return OpenGL texture ID or 0 if not ready
     */
    GLuint GetLatestTexture() const noexcept;

    /**
     * @brief Requests a new frame to be rendered
     */
    void RequestFrame();

    /**
     * @brief Associates the renderer with an Interface instance
     * @param interface Weak pointer to the Interface instance
     */
    void SetInterface(const std::weak_ptr<Interface>& interface) noexcept;

protected:
    /**
     * @brief Initializes rendering resources
     */
    void Initialize();

    /**
     * @brief Main rendering thread function
     */
    void RenderThreadMain();

    /**
     * @brief Updates texture contents from render graph output
     * @param buffer_index Index of the buffer to update
     */
    void UpdateTexture(unsigned int buffer_index);

protected:
    std::array<FrameBuffer, BUFFER_COUNT> buffers_;   ///< Triple buffered textures
    unsigned int current_write_index_ = 0;            ///< Current buffer for writing
    mutable unsigned int current_read_index_ = 0;     ///< Current buffer for reading

    std::unique_ptr<RenderGraph> render_graph_;       ///< Render graph instance
    ThreadPool thread_pool_;                          ///< Dedicated rendering thread

    std::atomic<bool> frame_requested_ = false;       ///< Frame request flag
    std::atomic<bool> stop_rendering_ = false;        ///< Termination flag
    mutable std::mutex render_graph_mutex_;           ///< Protects render graph access

    std::weak_ptr<Interface> interface_;              ///< Weak pointer to associated Interface instance
};

NAMESPACE_END(dream)