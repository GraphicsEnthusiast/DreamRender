#pragma once

#include <render_pass.h>
#include <thread_pool.h>

NAMESPACE_BEGIN(dream)

/**
 * @class RenderGraph
 * @brief Manages a directed acyclic graph (DAG) of rendering passes with explicit edge-based dependencies
 */
class RenderGraph {
public:
    /**
     * @brief Constructs render graph with context management
     * @param main_window Main GLFW window for context sharing
     */
    RenderGraph(GLFWwindow* main_window) : pool_(std::thread::hardware_concurrency()),
        next_handle_id_(0), main_window_(main_window) {}

    /**
     * @brief Registers a render pass with unique identifier
     * @param name Unique pass identifier
     * @param pass RenderPass instance (ownership transferred)
     */
    void AddPass(const std::string& name, std::unique_ptr<RenderPass> pass);

    /**
     * @brief Toggles enabled state of specified pass
     * @param name Pass identifier to modify
     * @param enabled New enablement state
     */
    void SetPassEnabled(const std::string& name, bool enabled);

    /**
     * @brief Connects two passes via named slots
     * @param src_pass Source pass name
     * @param src_output Source output slot name
     * @param dst_pass Destination pass name
     * @param dst_input Destination input slot name
     * @param access Required access type (default=Read)
     */
    void Connect(const std::string& src_pass, const std::string& src_output,
        const std::string& dst_pass, const std::string& dst_input,
        AccessType access = AccessType::Read);

    /**
     * @brief Compiles dependency graph based on explicit edges
     *
     * Compilation process:
     * 1. Processes all connection edges
     * 2. Builds resource-producer mapping
     * 3. Performs topological sort (Kahn's algorithm)
     * 4. Identifies final output texture
     */
    void Compile();

    /**
     * @brief Executes render passes with thread pool
     *
     * Execution workflow:
     * 1. Initializes task queue with topological order
     * 2. Builds reverse dependency map
     * 3. Launches worker threads that:
     *    - Wait for tasks with satisfied dependencies
     *    - Execute passes when ready
     *    - Update completion status atomically
     */
    void Execute();

    /**
     * @brief Provides access to final output texture
     * @return Texture handle or UINT32_MAX if undefined
     */
    TextureHandle GetFinalOutput() const noexcept;

    /**
     * @brief Assigns a dedicated context to a render pass
     * @param pass_name Name of the render pass
     * @param width Context width (default=1)
     * @param height Context height (default=1)
     */
    void AssignContextToPass(const std::string& pass_name, unsigned int width = 1, unsigned int height = 1);

protected:
    /**
     * @brief Retrieves pass pointer by name
     * @param name Pass identifier
     * @return RenderPass* or nullptr if not found
     */
    RenderPass* GetPass(const std::string& name);

protected:
    std::unordered_map<std::string, std::unique_ptr<RenderPass>> passes_;          ///< Name-indexed render passes
    std::vector<ResourceEdge> edges_;                                              ///< Explicit dependency connections
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> dependency_graph_;   ///< Adjacency list of dependencies
    std::vector<RenderPass*> execution_order_;                                     ///< Topologically sorted execution sequence
    TextureHandle final_output_;                                                   ///< Handle to final output texture
    ThreadPool pool_;                                                              ///< Thread pool for parallel execution
    unsigned int next_handle_id_;                                                  ///< Auto-generated texture handle counter
    GLFWwindow* main_window_;                                                      ///< Main GLFW window for context sharing
    std::unordered_map<std::string, std::shared_ptr<RenderContext>> pass_contexts_;///< Accessed during task execution to bind pass-specific context
};

NAMESPACE_END(dream)