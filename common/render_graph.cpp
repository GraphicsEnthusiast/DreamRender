#include <render_graph.h>

NAMESPACE_BEGIN(dream)

void RenderGraph::AddPass(const std::string& name, std::shared_ptr<RenderPass> pass) {
	passes_.insert({ name, pass });
}

void RenderGraph::AddEdge(const ResourceEdge& edge) {
	edges_.push_back(edge);
}

void RenderGraph::Compile() {
	// Step 1: Initialize in-degree map and adjacency list
	std::unordered_map<std::shared_ptr<RenderPass>, int> in_degree_map;
	std::unordered_map<std::shared_ptr<RenderPass>,
		std::vector<std::shared_ptr<RenderPass>>> adjacency_list;

	// Initialize all passes with zero in-degree
	for (auto& [name, pass] : passes_) {
		in_degree_map[pass] = 0;
		adjacency_list[pass] = {};
	}

	// Step 2: Process edges to build dependency graph
	for (const auto& edge : edges_) {
		// Locate source and destination passes in the passes map
		auto src_it = passes_.find(edge.src_pass);
		auto dst_it = passes_.find(edge.dst_pass);

		// Validate pass existence
		if (src_it == passes_.end() || dst_it == passes_.end()) {
			WARN("[warning] Skipping invalid edge: {}:{} -> {}:{}.",
				edge.src_pass.c_str(), edge.src_output.c_str(),
				edge.dst_pass.c_str(), edge.dst_input.c_str());
			continue;
		}

		// Get shared pointers to passes
		auto src_pass = src_it->second;
		auto dst_pass = dst_it->second;

		// Update adjacency list and in-degree counters
		adjacency_list[src_pass].push_back(dst_pass);
		in_degree_map[dst_pass]++;
	}

	// Step 3: Perform topological sort
	std::queue<std::shared_ptr<RenderPass>> zero_degree_queue;
	pass_execution_queue_.clear();

	// Initialize queue with passes having zero in-degree
	for (auto& [pass, degree] : in_degree_map) {
		if (0 == degree) {
			zero_degree_queue.push(pass);
		}
	}

	// Process the execution queue
	while (!zero_degree_queue.empty()) {
		auto current = zero_degree_queue.front();
		zero_degree_queue.pop();
		pass_execution_queue_.push_back(current);

		// Update neighbor in-degrees
		for (auto& neighbor : adjacency_list[current]) {
			if (0 == --in_degree_map[neighbor]) {
				zero_degree_queue.push(neighbor);
			}
		}
	}

	// Step 4: Cycle detection
	const unsigned int total_pass_count = static_cast<unsigned int>(passes_.size());
	const unsigned int sorted_pass_count = static_cast<unsigned int>(pass_execution_queue_.size());
	if (sorted_pass_count != total_pass_count) {
		std::string error_msg = "Render graph contains ";
		error_msg += (sorted_pass_count < total_pass_count) ?
			"disconnected passes" : "cyclic dependencies";
		error_msg += ". Total passes: " + std::to_string(total_pass_count);
		error_msg += ". Sorted passes: " + std::to_string(sorted_pass_count);
		ERROR("[error] {}.", error_msg.c_str());
	}
}

void RenderGraph::Execute() {
	for (auto& weak_pass : pass_execution_queue_) {
		if (auto pass = weak_pass.lock()) {
			//INFO("[info] {} execute.", pass->GetName().c_str());
			pass->Execute();
		}
		else {
			WARN("[warning] Skipping destroyed RenderPass in execution queue.");
		}
	}
	RenderPass::IncreaseFrameCounter();
}

void RenderGraph::SetFinalOutput(const TextureHandle& output) {
	output_ = output;
}

TextureHandle RenderGraph::GetFinalOutput() const noexcept {
	return output_;
}

NAMESPACE_END(dream)