#include <render_graph.h>

NAMESPACE_BEGIN(dream)

void RenderGraph::AddPass(const std::string& name, std::shared_ptr<RenderPass> pass) {
	passes_.emplace_back(name, std::move(pass));
	// Rebuild map immediately to prevent dangling pointers
	RebuildPassMap();
}

void RenderGraph::AddEdge(const ResourceEdge& edge) {
	edges_.push_back(edge);
}

void RenderGraph::RebuildPassMap() {
	pass_map_.clear();
	for (auto& [name, pass] : passes_) {
		pass_map_[name] = pass.get();
	}
}

void RenderGraph::Compile() {
	// Rebuild pass map to prevent dangling pointers
	RebuildPassMap();

	// Step 1: Build pass enablement map
	std::unordered_map<RenderPass*, bool> pass_enabled;
	for (auto& [name, pass] : passes_) {
		pass_enabled[pass.get()] = pass->IsEnabled();
	}

	// Step 2: Build effective edge relationships
	std::vector<ResourceEdge> effective_edges;
	for (const auto& edge : edges_) {
		// Validate pass names exist
		if (!pass_map_.count(edge.src_pass) || !pass_map_.count(edge.dst_pass)) {
			WARN("Skipping invalid edge: %s:%s -> %s:%s.",
				edge.src_pass.c_str(), edge.src_output.c_str(),
				edge.dst_pass.c_str(), edge.dst_input.c_str());
			continue;
		}

		RenderPass* src_pass = pass_map_[edge.src_pass];
		RenderPass* dst_pass = pass_map_[edge.dst_pass];

		// Only preserve edges between enabled passes
		if (pass_enabled[src_pass] && pass_enabled[dst_pass]) {
			effective_edges.push_back(edge);
		}
		// Skip disabled destination passes (no redirection)
		else if (pass_enabled[src_pass] && !pass_enabled[dst_pass]) {
			WARN("Disabled destination pass skipped: %s.", edge.dst_pass.c_str());
		}
	}

	// Step 3: Build dependency graph (enabled nodes only)
	std::unordered_map<RenderPass*, std::vector<RenderPass*>> adjacency_list;
	std::unordered_map<RenderPass*, int> in_degree_map;

	// Initialize only enabled passes
	int enabled_pass_count = 0;
	for (auto& [name, pass_ptr] : passes_) {
		if (!pass_enabled[pass_ptr.get()]) continue;
		in_degree_map[pass_ptr.get()] = 0;
		adjacency_list[pass_ptr.get()] = {};
		enabled_pass_count++;
	}

	// Build dependencies using effective edges
	for (const auto& edge : effective_edges) {
		// Revalidate passes (shouldn't happen but safe)
		if (!pass_map_.count(edge.src_pass) || !pass_map_.count(edge.dst_pass)) {
			continue;
		}

		RenderPass* src = pass_map_[edge.src_pass];
		RenderPass* dst = pass_map_[edge.dst_pass];

		// Ensure both ends are enabled (should be true by construction)
		if (pass_enabled[src] && pass_enabled[dst]) {
			adjacency_list[src].push_back(dst);
			in_degree_map[dst]++;
		}
	}

	// Step 4: Kahn's topological sort (enabled nodes only)
	std::queue<RenderPass*> zero_degree_queue;
	pass_execution_queue_.clear();

	// Initialize zero in-degree queue
	for (auto& [pass, degree] : in_degree_map) {
		if (0 == degree) {
			zero_degree_queue.push(pass);
		}
	}

	// Process queue
	while (!zero_degree_queue.empty()) {
		RenderPass* current = zero_degree_queue.front();
		zero_degree_queue.pop();
		pass_execution_queue_.push_back(current);

		// Update neighbor in-degrees
		for (RenderPass* neighbor : adjacency_list[current]) {
			if (0 == --in_degree_map[neighbor]) {
				zero_degree_queue.push(neighbor);
			}
		}
	}

	// Step 5: Cycle detection
	if (pass_execution_queue_.size() != enabled_pass_count) {
		// Improved error diagnostics
		std::string error_msg = "RenderGraph contains ";
		if (pass_execution_queue_.size() < enabled_pass_count) {
			error_msg += "disconnected enabled passes";
		}
		else {
			error_msg += "cyclic dependencies";
		}
		error_msg += ". Enabled passes: " + std::to_string(enabled_pass_count);
		error_msg += ", Sorted passes: " + std::to_string(pass_execution_queue_.size());
		ERROR("%s.", error_msg.c_str());
	}
}

void RenderGraph::Execute() {
	// No need for IsEnabled check - queue contains only enabled passes
	for (RenderPass* pass : pass_execution_queue_) {
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