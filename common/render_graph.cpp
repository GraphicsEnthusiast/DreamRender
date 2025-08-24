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

void RenderGraph::Compile() {
	// Rebuild pass map to prevent dangling pointers
	RebuildPassMap();

	// Step 1: Build a collection of enabled passes
	std::vector<std::shared_ptr<RenderPass>> enabled_passes;
	for (auto& [name, pass] : passes_) {
		if (pass->IsEnabled()) {
			enabled_passes.push_back(pass);
		}
	}

	// Step 2: Build effective edge relationships
	std::vector<ResourceEdge> effective_edges;
	for (const auto& edge : edges_) {
		// Validate source and destination passes exist in the pass map
		if (!pass_map_.count(edge.src_pass) || !pass_map_.count(edge.dst_pass)) {
			WARN("[warning] Skipping invalid edge: {}:{} -> {}:{}.",
				edge.src_pass.c_str(), edge.src_output.c_str(),
				edge.dst_pass.c_str(), edge.dst_input.c_str());
			continue;
		}

		// Resolve weak pointers to shared pointers
		auto src_pass = pass_map_[edge.src_pass].lock();
		auto dst_pass = pass_map_[edge.dst_pass].lock();

		// Skip edges referencing destroyed passes
		if (!src_pass || !dst_pass) {
			WARN("[warning] Edge references destroyed pass: {} -> {}",
				edge.src_pass.c_str(), edge.dst_pass.c_str());
			continue;
		}

		// Preserve edges only between enabled passes
		if (src_pass->IsEnabled() && dst_pass->IsEnabled()) {
			effective_edges.push_back(edge);
		}
		// Log skipped edges due to disabled destination pass
		else if (src_pass->IsEnabled() && !dst_pass->IsEnabled()) {
			WARN("[warning] Disabled destination pass skipped: {}.", edge.dst_pass.c_str());
		}
	}

	// Step 3: Build dependency graph using shared ownership
	std::unordered_map<std::shared_ptr<RenderPass>, std::vector<std::shared_ptr<RenderPass>>> adjacency_list;
	std::unordered_map<std::shared_ptr<RenderPass>, int> in_degree_map;

	// Initialize adjacency list and in-degree counters for enabled passes
	for (auto& pass : enabled_passes) {
		in_degree_map[pass] = 0;
		adjacency_list[pass] = {};
	}

	// Populate adjacency list and in-degree map from effective edges
	for (const auto& edge : effective_edges) {
		auto src_pass = pass_map_[edge.src_pass].lock();
		auto dst_pass = pass_map_[edge.dst_pass].lock();

		// Add edge only if both passes are valid
		if (src_pass && dst_pass) {
			adjacency_list[src_pass].push_back(dst_pass);
			in_degree_map[dst_pass]++;
		}
	}

	// Step 4: Perform Kahn's topological sort
	std::queue<std::shared_ptr<RenderPass>> zero_degree_queue;
	pass_execution_queue_.clear();

	// Initialize queue with passes having zero in-degree
	for (auto& [pass, degree] : in_degree_map) {
		if (degree == 0) {
			zero_degree_queue.push(pass);
		}
	}

	// Process the queue to determine execution order
	while (!zero_degree_queue.empty()) {
		auto current = zero_degree_queue.front();
		zero_degree_queue.pop();
		pass_execution_queue_.push_back(current); // Store as weak_ptr

		// Decrement in-degree of neighbors and enqueue if zero
		for (auto& neighbor : adjacency_list[current]) {
			if (--in_degree_map[neighbor] == 0) {
				zero_degree_queue.push(neighbor);
			}
		}
	}

	// Step 5: Cycle detection
	const unsigned int enabled_pass_count = enabled_passes.size();
	const unsigned int execution_pass_count = pass_execution_queue_.size();
	if (execution_pass_count != enabled_pass_count) {
		// Improved error diagnostics
		std::string error_msg = "RenderGraph contains ";
		if (execution_pass_count < enabled_pass_count) {
			error_msg += "disconnected enabled passes";
		}
		else {
			error_msg += "cyclic dependencies";
		}
		error_msg += ". Enabled passes: " + std::to_string(enabled_pass_count);
		error_msg += ", Sorted passes: " + std::to_string(execution_pass_count);
		ERROR("[error] {}.", error_msg.c_str());
	}
}

void RenderGraph::Execute() {
	static bool is_first = true;
	for (auto& weak_pass : pass_execution_queue_) {
		if (auto pass = weak_pass.lock()) {
			if (is_first) {
				INFO("[info] {} execute.", pass->GetName().c_str());
				is_first = false;
			}
			pass->Execute();
		}
		else {
			WARN("[warning] Skipping destroyed RenderPass in execution queue.");
		}
	}
}

void RenderGraph::SetFinalOutput(const TextureHandle& output) {
	output_ = output;
}

TextureHandle RenderGraph::GetFinalOutput() const noexcept {
	return output_;
}

void RenderGraph::RebuildPassMap() {
	pass_map_.clear();
	for (auto& [name, pass] : passes_) {
		pass_map_[name] = pass;
	}
}

NAMESPACE_END(dream)