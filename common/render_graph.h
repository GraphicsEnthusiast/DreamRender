#pragma once

#include <render_pass.h>

NAMESPACE_BEGIN(dream)

/**
 * @class RenderGraph
 * @brief Manages a directed acyclic graph (DAG) of rendering passes for serial execution
 */
class RenderGraph {
public:
    /**
     * @brief Constructs a RenderGraph object
     *
     * Initializes the render graph with default values:
     * - Sets next texture handle ID to 0
     * - Marks final output as invalid (UINT32_MAX)
     */
    RenderGraph() : next_handle_id_(0) {}

    /**
     * @brief Registers a render pass with a unique identifier
     * @param name Unique name for the render pass
     * @param pass Unique pointer to the RenderPass instance (ownership transferred to graph)
     *
     * @note The graph takes ownership of the RenderPass object
     */
    void AddPass(const std::string& name, std::unique_ptr<RenderPass> pass);

    /**
     * @brief Sets the enabled state of a specific render pass
     * @param name Name of the pass to modify
     * @param enabled New enablement state (true = active, false = disabled)
     *
     * @note Disabled passes will be skipped during compilation and execution
     */
    void SetPassEnabled(const std::string& name, bool enabled);

    /**
     * @brief Connects two render passes via their named resource slots
     * @param src_pass Source pass name
     * @param src_output Output slot name of the source pass
     * @param dst_pass Destination pass name
     * @param dst_input Input slot name of the destination pass
     *
     * Creates a resource dependency where the destination pass consumes
     * a resource produced by the source pass.
     */
    void Connect(const std::string& src_pass, const std::string& src_output,
        const std::string& dst_pass, const std::string& dst_input);

    /**
     * @brief Compiles the dependency graph
     *
     * Performs the following operations:
     * 1. Processes all connection edges
     * 2. Assigns texture handles to resources
     * 3. Builds dependency graph
     * 4. Performs topological sort to determine execution order
     * 5. Identifies the final output texture
     *
     * @note Must be called after adding all passes and connections
     *       and before executing the graph
     */
    void Compile();

    /**
     * @brief Executes all enabled render passes in topological order
     *
     * Iterates through the compiled execution order and calls
     * the Execute() method of each enabled pass.
     */
    void Execute();

    /**
     * @brief Retrieves the handle to the final output texture
     * @return TextureHandle of the final output, or invalid handle (UINT32_MAX)
     *         if no final output is defined
     */
    TextureHandle GetFinalOutput() const noexcept;

protected:
    /**
     * @brief Retrieves a render pass by name
     * @param name Name of the pass to retrieve
     * @return Pointer to RenderPass, or nullptr if not found
     */
    RenderPass* GetPass(const std::string& name);

protected:
    std::unordered_map<std::string, std::unique_ptr<RenderPass>> passes_;         ///< Map of pass names to RenderPass instances
    std::vector<ResourceEdge> edges_;                                             ///< List of explicit resource connections
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> dependency_graph_;  ///< Adjacency list representing pass dependencies
    std::vector<RenderPass*> execution_order_;                                    ///< Topologically sorted execution sequence
    TextureHandle final_output_;                                                  ///< Handle to the final output texture
    unsigned int next_handle_id_;                                                 ///< Counter for generating unique texture handles
};

NAMESPACE_END(dream)