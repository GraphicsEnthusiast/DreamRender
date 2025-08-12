#pragma once

#include <render_pass.h>
#include <thread_pool.h>

NAMESPACE_BEGIN(dream)

/**
 * @class RenderGraph
 * @brief Manages a directed acyclic graph (DAG) of rendering passes with dependency resolution
 *
 * The RenderGraph orchestrates execution of rendering passes based on their resource dependencies.
 * It provides mechanisms for:
 * - Adding/removing rendering passes
 * - Managing pass enable/disable states
 * - Compiling dependency graphs
 * - Executing passes in topological order
 * - Parallel execution via thread pooling
 */
class RenderGraph {
public:
    /// Initializes thread pool with hardware-concurrency count
    RenderGraph() : pool_(std::thread::hardware_concurrency()) {}

    /// Registers a render pass with unique name
    void AddPass(const std::string& name, std::unique_ptr<RenderPass> pass);

    /// Enables/disables specified render pass
    void SetPassEnabled(const std::string& name, bool enabled);

    /**
     * Compiles render graph by:
     * 1. Identifying resource producers
     * 2. Building dependency graph
     * 3. Performing topological sort
     * 4. Determining final output texture
     */
    void Compile();

    /**
     * Executes render passes with:
     * - Dependency-resolved ordering
     * - Thread-parallel execution
     * - Condition variable synchronization
     */
    void Execute();

    /// Retrieves final output texture handle
    TextureHandle GetFinalOutput() const noexcept;

private:
    std::unordered_map<std::string, std::unique_ptr<RenderPass>> passes_;         ///< Name-indexed render passes
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> dependency_graph_;  ///< Adjacency list of pass dependencies
    std::vector<RenderPass*> execution_order_;                                    ///< Topologically sorted execution sequence
    TextureHandle final_output_;                                                  ///< Handle to final output texture
    ThreadPool pool_;                                                             ///< Thread pool for parallel execution
};

NAMESPACE_END(dream)