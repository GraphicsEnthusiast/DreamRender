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
	// Step 1: Build pass enablement map
	std::unordered_map<RenderPass*, bool> pass_enabled;
	for (auto& [name, pass] : passes_) {
		pass_enabled[pass.get()] = pass->IsEnabled();
	}

	// Step 2: Build effective edge relationships
	std::vector<ResourceEdge> effective_edges;
	for (const auto& edge : edges_) {
		RenderPass* src_pass = pass_map_[edge.src_pass];
		RenderPass* dst_pass = pass_map_[edge.dst_pass];

		// Scenario 1: Both passes enabled ¡ú preserve original edge
		if (pass_enabled[src_pass] && pass_enabled[dst_pass]) {
			effective_edges.push_back(edge);
		}
		// Scenario 2: Destination pass disabled ¡ú redirect to next enabled node
		else if (pass_enabled[src_pass] && !pass_enabled[dst_pass]) {
			bool redirected = false;

			// Find all downstream nodes of dst_pass
			for (const auto& next_edge : edges_) {
				if (next_edge.src_pass == edge.dst_pass) {
					RenderPass* next_dst = pass_map_[next_edge.dst_pass];

					// Redirect only to ENABLED downstream nodes
					if (pass_enabled[next_dst]) {
						effective_edges.push_back({
							edge.src_pass, edge.src_output,
							next_edge.dst_pass, next_edge.dst_input
							});
						redirected = true;
					}
				}
			}

			// Scenario 3: No enabled downstream ¡ú mark as final output
			if (!redirected) {
				WARN("Disconnected output from disabled pass: %s.", edge.dst_pass.c_str());
				SetFinalOutput(src_pass->GetOutputTexture(edge.src_output));
			}
		}
	}

	// Step 3: Build dependency graph (enabled nodes only)
	std::unordered_map<RenderPass*, std::vector<RenderPass*>> adjacency_list;
	std::unordered_map<RenderPass*, int> in_degree_map;

	// Initialize only enabled passes
	int enabled_pass_count = 0;
	for (auto& [name, pass_ptr] : passes_) {
		if (!pass_enabled[pass_ptr.get()]) {
			continue;
		}
		in_degree_map[pass_ptr.get()] = 0;
		adjacency_list[pass_ptr.get()] = {};
		enabled_pass_count++;
	}

	// Build dependencies using effective edges
	for (const auto& edge : effective_edges) {
		RenderPass* src = pass_map_[edge.src_pass];
		RenderPass* dst = pass_map_[edge.dst_pass];

		// Ensure both ends are enabled
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
		ERROR("RenderGraph contains cyclic dependencies or disconnected enabled passes.");
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