#include <render_graph.h>

NAMESPACE_BEGIN(dream)

void RenderGraph::AddPass(const std::string& name, std::shared_ptr<RenderPass> pass) {
	// Transfer shared ownership to passes vector
	passes_.emplace_back(name, std::move(pass));

	// Store raw pointer for efficient lookups (ownership retained in passes_)
	pass_map_[name] = passes_.back().second.get();
}

void RenderGraph::AddEdge(const ResourceEdge& edge) {
	edges_.push_back(edge);
}

void RenderGraph::Compile() {
	// Build adjacency list and in-degree counters
	std::unordered_map<RenderPass*, std::vector<RenderPass*>> adjacency_list;
	std::unordered_map<RenderPass*, int> in_degree_map;

	// Initialize all passes with zero in-degree
	for (const auto& [name, pass_ptr] : passes_) {
		RenderPass* pass = pass_ptr.get();
		in_degree_map[pass] = 0;
		adjacency_list[pass] = {};
	}

	// Process edges to build dependencies
	for (const auto& edge : edges_) {
		auto src_it = pass_map_.find(edge.src_pass);
		auto dst_it = pass_map_.find(edge.dst_pass);

		if (pass_map_.end() != src_it && pass_map_.end() != dst_it) {
			// Establish dependency: src_pass must execute before dst_pass
			adjacency_list[src_it->second].push_back(dst_it->second);
			in_degree_map[dst_it->second]++;
		}
	}

	// Kahn's algorithm for topological sort
	std::queue<RenderPass*> zero_degree_queue;
	pass_execution_queue_.clear();

	// Initialize queue with zero in-degree passes
	for (const auto& [pass, degree] : in_degree_map) {
		if (0 == degree) {
			zero_degree_queue.push(pass);
		}
	}

	// Process the queue
	while (!zero_degree_queue.empty()) {
		RenderPass* current = zero_degree_queue.front();
		zero_degree_queue.pop();

		pass_execution_queue_.push_back(current);

		// Update neighbors' in-degree
		for (RenderPass* neighbor : adjacency_list[current]) {
			if (0 == --in_degree_map[neighbor]) {
				zero_degree_queue.push(neighbor);
			}
		}
	}

	// Cycle detection
	if (pass_execution_queue_.size() != passes_.size()) {
		ERROR("RenderGraph contains cyclic dependencies.");
	}
}

void RenderGraph::Execute() {
	for (RenderPass* pass : pass_execution_queue_) {
		if (!pass->IsEnabled()) {
			continue;
		}

		pass->Execute();
	}
}

void RenderGraph::SetFinalOutput(const TextureHandle& output) {
	output_ = output;
}

TextureHandle RenderGraph::GetFinalOutput() const noexcept {
	return output_;
}


NAMESPACE_END(dream)