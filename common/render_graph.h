#pragma once

#include <render_pass.h>

NAMESPACE_BEGIN(dream)

/**
 * @class RenderGraph
 * @brief Manages serial execution of render passes with explicit resource dependencies
 */
class RenderGraph {
public:
	RenderGraph() = default;
	~RenderGraph() = default;

	/**
	 * @brief Adds a render pass to the graph with shared ownership
	 * @param name Unique identifier for the pass
	 * @param pass Shared pointer to the RenderPass instance
	 */
	void AddPass(const std::string& name, std::shared_ptr<RenderPass> pass);

	/**
	 * @brief Defines resource dependency between passes
	 * @param edge ResourceEdge specifying source and destination
	 */
	void AddEdge(const ResourceEdge& edge);

	/**
	 * @brief Compiles the graph by resolving dependencies
	 */
	void Compile();

	/**
	 * @brief Executes enabled passes in topological order
	 */
	void Execute();

	/**
	 * @brief Set the final output texture handle
	 */
	void SetFinalOutput(const TextureHandle& output);

	/**
	 * @brief Retrieves the final output texture handle from the render graph
	 * @return TextureHandle
	 */
	TextureHandle GetFinalOutput() const noexcept;
protected:
	/**
	 * @brief Rebuilds the pass name to pointer mapping from current passes_
	 */
	void RebuildPassMap();

protected:
	std::vector<std::pair<std::string, std::shared_ptr<RenderPass>>> passes_;   ///< Execution sequence with shared ownership
	std::unordered_map<std::string, std::weak_ptr<RenderPass>> pass_map_;       ///< Pass lookup by name (non-owning references)
	std::vector<ResourceEdge> edges_;                                           ///< Dependency definitions
    std::vector<std::weak_ptr<RenderPass>> pass_execution_queue_;               ///< Topologically sorted passes (non-owning pointers)
	TextureHandle output_;                                                      ///< Graph output
};

NAMESPACE_END(dream)