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
    const std::string& dst_pass, const std::string& dst_input,
    AccessType access) {
    edges_.push_back({ src_pass, src_output, dst_pass, dst_input, access });
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
        dst_pass->inputs_.push_back({ handle, edge.access });

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
        if (pass->is_final_output_ && !pass->outputs_.empty()) {
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
    std::mutex task_mutex;
    std::condition_variable cv;
    std::atomic<bool> all_tasks_completed = false;

    // Initialize task queue with topological order
    std::list<RenderPass*> ready_tasks(execution_order_.begin(), execution_order_.end());

    // Build reverse dependency map [consumer -> producers]
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> reverse_deps;
    for (auto& [producer, consumers] : dependency_graph_) {
        for (auto* consumer : consumers) {
            reverse_deps[consumer].push_back(producer);
        }
    }

    // Atomic task counter for completion tracking
    std::atomic<unsigned int> completed_count = 0;
    const unsigned int total_tasks = static_cast<unsigned int>(execution_order_.size());

    // Worker task function
    auto worker_task = [&] {
        while (!all_tasks_completed) {
            RenderPass* task = nullptr;

            {
                std::unique_lock<std::mutex> lock(task_mutex);

                // Wait for ready task or completion signal
                cv.wait(lock, [&] {
                    if (ready_tasks.empty() || all_tasks_completed) {
                        return true;
                    }

                    // Find task with satisfied dependencies
                    for (auto it = ready_tasks.begin(); it != ready_tasks.end(); ++it) {
                        bool deps_met = true;

                        // Check all producer dependencies
                        if (auto deps = reverse_deps.find(*it); deps != reverse_deps.end()) {
                            for (auto* dep : deps->second) {
                                if (!dep->IsCompleted()) {
                                    deps_met = false;

                                    break;
                                }
                            }
                        }

                        if (deps_met) {
                            task = *it;
                            ready_tasks.erase(it);

                            return true;
                        }
                    }

                    return false;
                    });

                if (all_tasks_completed) {
                    return;
                }
            }

            if (task) {
                // Execute pass and mark completion
                task->Execute();
                task->completed_.store(true);

                // Update completion counter
                {
                    std::lock_guard<std::mutex> lock(task_mutex);
                    completed_count++;
                }
                cv.notify_all();
            }
        }
    };

    // Submit tasks to thread pool
    const unsigned int num_workers = std::min(
        static_cast<unsigned int>(execution_order_.size()),
        static_cast<unsigned int>(std::thread::hardware_concurrency())
    );

    for (unsigned int i = 0; i < num_workers; ++i) {
        pool_.Enqueue(worker_task);
    }

    // Wait for all tasks to complete
    while (completed_count < total_tasks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Signal threads to exit
    {
        std::lock_guard<std::mutex> lock(task_mutex);
        all_tasks_completed = true;
    }
    cv.notify_all();
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