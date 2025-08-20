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

protected:
    std::unique_ptr<RenderGraph> graph_;  ///< Managed render graph instance
    GLFWwindow* render_context_;          ///< Dedicated OpenGL context
};

/**
 * @class TestPipeline
 * @brief Concrete implementation of RenderPipeline for testing purposes
 */
class TestPipeline : public RenderPipeline {
public:
    TestPipeline(GLFWwindow* share_window = nullptr) : RenderPipeline(share_window) {}

    /**
         * @brief Configures the test pipeline with processing and presentation passes
         */
    virtual void Init() override {
        // Create processing pass (simulates color transformation)
        auto processor = std::make_shared<ColorProcessingPass>();

        // Create presentation pass (simulates final rendering)
        auto presenter = std::make_shared<PresentPass>();

        // Create test textures (simulated resources)
        TextureHandle input_tex = CreateColorTexture(512, 512);
        TextureHandle processed_tex = CreateColorTexture(512, 512);
        TextureHandle output_tex = CreateColorTexture(512, 512);

        // Configure pass inputs/outputs
        processor->SetInputTexture("Input", input_tex);
        processor->SetOutputTexture("Output", processed_tex);
        presenter->SetInputTexture("ScreenInput", processed_tex);
        presenter->SetOutputTexture("ScreenOutput", output_tex);

        // Register passes in the render graph
        AddPass("ColorProcessor", processor);
        AddPass("FinalPresenter", presenter);

        // Define resource dependencies
        ConnectPasses(
            "ColorProcessor", "Output",
            "FinalPresenter", "ScreenInput"
        );

        // Designate final output
        SetFinalOutput(output_tex);
    }
};

NAMESPACE_END(dream)