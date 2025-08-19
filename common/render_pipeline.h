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
     */
    RenderPipeline();

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

protected:
    /**
     * @brief Pure virtual method for pipeline configuration
     *
     * Derived classes must implement this to:
     * 1. Add render passes to the graph
     * 2. Define resource dependencies between passes
     * 3. Set the final output texture
     */
    virtual void Setup() = 0;

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
};

NAMESPACE_END(dream)