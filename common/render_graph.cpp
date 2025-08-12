#include <render_graph.h>

NAMESPACE_BEGIN(dream)

/**
 * @brief Registers a render pass with unique identifier
 * @param name Unique pass identifier
 * @param pass RenderPass instance (ownership transferred)
 */
void RenderGraph::AddPass(const std::string& name, std::unique_ptr<RenderPass> pass) {
    passes_.emplace(name, std::move(pass));
}

/**
 * @brief Toggles enabled state of specified pass
 * @param name Pass identifier to modify
 * @param enabled New enablement state
 */
void RenderGraph::SetPassEnabled(const std::string& name, bool enabled) {
    auto it = passes_.find(name);
    if (passes_.end() != it) {
        it->second->SetEnabled(enabled);
    }
}

/**
 * @brief Compiles render graph dependencies and execution order
 *
 * Compilation process:
 * 1. Resets previous compilation state
 * 2. Identifies resource producers (texture -> pass mapping)
 * 3. Builds dependency graph (producer -> consumer edges)
 * 4. Performs Kahn's algorithm for topological sorting
 * 5. Identifies final output texture from designated passes
 */
void RenderGraph::Compile() {
    /// Initialize final output as invalid handle
    final_output_ = TextureHandle{ UINT32_MAX };
    dependency_graph_.clear();
    execution_order_.clear();

    /// Custom hash for TextureHandle to enable unordered_map usage
    struct TextureHandleHash {
        std::size_t operator()(const TextureHandle& handle) const noexcept {
            return std::hash<uint32_t>{}(handle.id);
        }
    };

    /// Maps texture resources to their producer passes
    std::unordered_map<TextureHandle, RenderPass*, TextureHandleHash> resource_producers;

    /// First pass: identify all enabled passes and their output resources
    for (auto& [name, pass] : passes_) {
        if (!pass->IsEnabled()) {
            continue;
        }
        // Record producer for each output texture
        for (auto& output : pass->outputs_) {
            resource_producers[output] = pass.get();
        }
    }

    /// Second pass: build dependency graph between passes
    for (auto& [name, pass] : passes_) {
        if (!pass->IsEnabled()) {
            continue;
        }
        /// For each input dependency
        for (auto& input : pass->inputs_) {
            if (auto producer = resource_producers.find(input.resource);
                producer != resource_producers.end()) {
                /// Add edge: producer -> current pass
                dependency_graph_[producer->second].push_back(pass.get());
            }
        }
    }

    /// Prepare for topological sort (Kahn's algorithm)
    std::unordered_map<RenderPass*, int> in_degree;  // Tracks dependency count per pass
    std::queue<RenderPass*> ready_queue;  // Queue for passes with zero dependencies

    /// Initialize in-degree counts for all enabled passes
    for (auto& [name, pass] : passes_) {
        if (!pass->IsEnabled()) {
            continue;
        }
        in_degree[pass.get()] = 0;
    }

    /// Calculate initial in-degrees from dependency graph edges
    for (auto& [node, dependents] : dependency_graph_) {
        for (auto* dep : dependents) {
            in_degree[dep]++;
        }
    }

    /// Identify passes with zero dependencies (starting points)
    for (auto& [pass, degree] : in_degree) {
        if (degree == 0) {
            ready_queue.push(pass);
        }
    }

    /// Process nodes in topological order
    while (!ready_queue.empty()) {
        auto* pass = ready_queue.front();
        ready_queue.pop();
        execution_order_.push_back(pass);  // Add to execution sequence

        /// Identify final output from designated passes
        if (pass->is_final_output_ && !pass->outputs_.empty()) {
            final_output_ = pass->outputs_[0];
        }

        /// Update dependencies and queue newly ready passes
        if (auto it = dependency_graph_.find(pass); it != dependency_graph_.end()) {
            for (auto* dependent : it->second) {
                if (--in_degree[dependent] == 0) {
                    ready_queue.push(dependent);
                }
            }
        }
    }
}

/**
 * @brief Executes render passes using thread pool with dependency enforcement
 *
 * Execution workflow:
 * 1. Initializes task queue with topological order
 * 2. Builds reverse dependency map (consumer -> producers)
 * 3. Submits tasks to thread pool that:
 *    - Wait for dependencies using condition variables
 *    - Execute passes when dependencies are met
 *    - Update task completion status atomically
 */
void RenderGraph::Execute() {
    std::mutex task_mutex;
    std::condition_variable cv;
    bool all_tasks_completed = false;

    /// Initialize task queue with topological order
    std::list<RenderPass*> ready_tasks(execution_order_.begin(), execution_order_.end());

    /// Build reverse dependency map (consumers -> producers)
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> dependency_map;
    for (auto& [producer, consumers] : dependency_graph_) {
        for (auto* consumer : consumers) {
            dependency_map[consumer].push_back(producer);
        }
    }

    /// Atomic task counter for completion tracking
    std::atomic<unsigned int> completed_count = 0;
    const unsigned int total_tasks = execution_order_.size();

    /// Worker task function (adapted for thread pool)
    auto task_func = [&] {
        while (true) {
            RenderPass* task = nullptr;
            {
                std::unique_lock<std::mutex> lock(task_mutex);
                cv.wait(lock, [&] {
                    if (ready_tasks.empty()) {
                        return true;
                    }

                    for (auto it = ready_tasks.begin(); it != ready_tasks.end(); ++it) {
                        bool dependencies_met = true;
                        if (auto deps = dependency_map.find(*it); deps != dependency_map.end()) {
                            for (auto* dep : deps->second) {
                                if (!dep->IsCompleted()) {
                                    dependencies_met = false;
                                    break;
                                }
                            }
                        }

                        if (dependencies_met) {
                            task = *it;
                            ready_tasks.erase(it);
                            return true;
                        }
                    }
                    return false;
                    });

                if (ready_tasks.empty() && completed_count == total_tasks) {
                    all_tasks_completed = true;

                    return; /// Exit task on completion
                }
                if (!task) {
                    return; /// No ready tasks
                }
            }

            /// Execute pass and mark completion
            task->Execute();
            task->completed_.store(true);

            /// Update completion counter and notify
            {
                std::lock_guard<std::mutex> lock(task_mutex);
                completed_count++;
            }
            cv.notify_all(); /// Wake waiting workers
        }
    };

    /// Submit tasks to thread pool (1 task per thread)
    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        pool_.Enqueue(task_func);
    }

    /// Wait for all tasks to complete
    while (completed_count < total_tasks) {
        std::this_thread::yield();
    }

    /// Signal threads to exit
    {
        std::lock_guard<std::mutex> lock(task_mutex);
        all_tasks_completed = true;
        cv.notify_all();
    }
}

/**
 * @brief Provides access to final output texture
 * @return Texture handle or UINT32_MAX if undefined
 */
TextureHandle RenderGraph::GetFinalOutput() const noexcept {
    return final_output_;
}

NAMESPACE_END(dream)