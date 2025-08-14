#include <render_graph.h>

NAMESPACE_BEGIN(dream)

void RenderGraph::AddPass(const std::string& name, std::unique_ptr<RenderPass> pass) {
    passes_.emplace(name, std::move(pass));
}

void RenderGraph::SetPassEnabled(const std::string& name, bool enabled) {
    if (auto it = passes_.find(name); passes_.end() != it) {
        it->second->SetEnabled(enabled);
    }
}

void RenderGraph::Connect(const std::string& src_pass, const std::string& src_output,
    const std::string& dst_pass, const std::string& dst_input) {
    edges_.push_back({ src_pass, src_output, dst_pass, dst_input });
}

void RenderGraph::Compile() {
    final_output_ = TextureHandle{ UINT32_MAX };
    dependency_graph_.clear();
    execution_order_.clear();

    // Resource mapping table [resource_id] -> (producer_pass, texture_handle)
    std::unordered_map<std::string, std::pair<RenderPass*, TextureHandle>> resource_map;

    // Process all connection edges
    for (auto& edge : edges_) {
        auto* src_pass = GetPass(edge.src_pass);
        auto* dst_pass = GetPass(edge.dst_pass);

        // Skip connections involving disabled passes
        if (!src_pass || !dst_pass || !src_pass->IsEnabled() || !dst_pass->IsEnabled()) {
            continue;
        }

        // Create resource identifier (format: PassName::SlotName)
        const std::string resource_id = edge.src_pass + "::" + edge.src_output;

        // Create or retrieve texture handle
        if (!resource_map.count(resource_id)) {
            resource_map[resource_id] = { src_pass, TextureHandle{next_handle_id_++} };
        }
        auto& [producer, handle] = resource_map[resource_id];

        // Add to producer outputs
        if (producer->outputs_.end() ==
            std::find(producer->outputs_.begin(), producer->outputs_.end(), handle)) {
            producer->outputs_.push_back(handle);
        }

        // Add to consumer inputs
        dst_pass->inputs_.push_back({ handle });

        // Add to dependency graph
        dependency_graph_[producer].push_back(dst_pass);
    }

    // Topological sort using Kahn's algorithm
    std::unordered_map<RenderPass*, int> in_degree;
    std::queue<RenderPass*> ready_queue;

    // Initialize in-degree counts for enabled passes
    for (auto& [name, pass] : passes_) {
        if (pass->IsEnabled()) {
            in_degree[pass.get()] = 0;
        }
    }

    // Calculate initial in-degrees from dependency edges
    for (auto& [producer, consumers] : dependency_graph_) {
        for (auto* consumer : consumers) {
            in_degree[consumer]++;
        }
    }

    // Find passes with zero dependencies (starting points)
    for (auto& [pass, degree] : in_degree) {
        if (0 == degree) {
            ready_queue.push(pass);
        }
    }

    // Process nodes in topological order
    while (!ready_queue.empty()) {
        auto* pass = ready_queue.front();
        ready_queue.pop();
        execution_order_.push_back(pass);

        // Identify final output from designated passes
        if (pass->is_final_output_&& !pass->outputs_.empty()) {
            final_output_ = pass->outputs_.front();
        }

        // Update dependencies and enqueue newly ready passes
        if (auto it = dependency_graph_.find(pass); dependency_graph_.end() != it) {
            for (auto* consumer : it->second) {
                if (0 == --in_degree[consumer]) {
                    ready_queue.push(consumer);
                }
            }
        }
    }
}

void RenderGraph::Execute() {
    // Simply execute all passes in topological order
    for (RenderPass* pass : execution_order_) {
        if (pass->IsEnabled()) {
            pass->Execute();
        }
    }
}

TextureHandle RenderGraph::GetFinalOutput() const noexcept {
    return final_output_;
}

RenderPass* RenderGraph::GetPass(const std::string& name) {
    if (auto it = passes_.find(name); passes_.end() != it) {
        return it->second.get();
    }

    return nullptr;
}

NAMESPACE_END(dream)