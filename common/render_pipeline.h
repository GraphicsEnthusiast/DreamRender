#pragma once

#include <render_graph.h>

NAMESPACE_BEGIN(dream)

/**
 * @class RenderPipeline
 * @brief Base class for constructing render pipelines by connecting render passes
 */
class RenderPipeline {
public:
    /**
     * @brief Constructs a RenderPipeline and initializes the internal render graph
     * @param share_window Main window context for resource sharing (optional)
     */
    explicit RenderPipeline(GLFWwindow* share_window = nullptr);

    /**
     * @brief Destructor - releases OpenGL context resources
     */
    virtual ~RenderPipeline();

    /**
     * @brief Pure virtual method for pipeline configuration
     *
     * Derived classes must implement this to:
     * 1. Add render passes to the graph
     * 2. Define resource dependencies between passes
     * 3. Set the final output texture
     */
    virtual void Init() = 0;

    /**
     * @brief Executes the compiled render pipeline
     */
    void Execute();

    /**
     * @brief Retrieves the final output texture from the pipeline
     * @return TextureHandle to the final output
     */
    TextureHandle GetFinalOutput() const noexcept;

    /**
     * @brief Compiles the render graph for execution
     */
    void Compile();

    /**
     * @brief Makes the pipeline's OpenGL context current for the calling thread
     */
    void MakeContextCurrent();

    /**
     * @brief Gets the current rendering size (width and height) for the pipeline.
     * @return const Point2f& The current rendering size.
     */
    const Point2i& GetRenderingSize() const noexcept;

    /**
     * @brief Sets the rendering size for the pipeline.
     * @param size The new rendering size (width and height).
     */
    void SetRenderingSize(const Point2i& size);

protected:
    /**
     * @brief Adds a pass to the pipeline with ownership transfer
     * @param name Unique identifier for the pass
     * @param pass Shared pointer to the RenderPass instance
     */
    void AddPass(const std::string& name, std::shared_ptr<RenderPass> pass);

    /**
     * @brief Connects two passes via a resource dependency
     * @param src_pass Source pass name
     * @param src_output Source output slot name
     * @param dst_pass Destination pass name
     * @param dst_input Destination input slot name
     */
    void ConnectPasses(const std::string& src_pass, const std::string& src_output,
        const std::string& dst_pass, const std::string& dst_input);

    /**
     * @brief Marks a texture as the pipeline's final output
     * @param output TextureHandle to use as final output
     */
    void SetFinalOutput(const TextureHandle& output);

    /**
	 * @brief Creates a new 2D texture with specified dimensions and default parameters.
	 * @param width  The width of the texture in pixels (must be positive).
	 * @param height The height of the texture in pixels (must be positive).
	 * @return TextureHandle  Wrapper containing the OpenGL texture ID.
	 */
    TextureHandle CreateTexture(int width, int height);

protected:
    std::unique_ptr<SceneManager> mesh_manager_;
    std::unique_ptr<RenderGraph> graph_;
    GLFWwindow* render_window_;           ///< Dedicated OpenGL context
    Point2i rendering_size_;              ///< The rendering size (width and height) for the pipeline
};

/**
 * @class PTPipeline
 * @brief Concrete implementation of RenderPipeline for testing purposes
 */
class PTPipeline : public RenderPipeline {
public:
    PTPipeline(GLFWwindow* share_window = nullptr) : RenderPipeline(share_window) {}

    /**
     * @brief Configures the path tracing pipeline
     */
    void Init() override;
};

/**
 * @class LTPipeline
 * @brief Concrete implementation of RenderPipeline for Light Tracing
 */
class LTPipeline : public RenderPipeline {
public:
    LTPipeline(GLFWwindow* share_window = nullptr) : RenderPipeline(share_window) {}

    /**
     * @brief Configures the light tracing pipeline
     */
    void Init() override;
};

NAMESPACE_END(dream)