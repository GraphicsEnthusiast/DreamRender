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
 * @brief Executes render passes with thread-parallel processing
 *
 * Execution workflow:
 * 1. Initializes task queue with topological order
 * 2. Creates reverse dependency map (consumer -> producers)
 * 3. Launches worker threads that:
 *    - Wait for tasks with satisfied dependencies
 *    - Execute passes when dependencies are met
 *    - Update completion status atomically
 * 4. Uses condition variables for task synchronization
 */
void RenderGraph::Execute() {
    std::vector<std::thread> workers;
    std::mutex task_mutex;                 /// Synchronizes task queue access
    std::condition_variable cv;            /// Coordinates task scheduling
    unsigned int completed_count = 0;      /// Tracks finished tasks

    /// Initialize task queue with topological order
    std::list<RenderPass*> ready_tasks(execution_order_.begin(), execution_order_.end());

    /// Build reverse dependency map (consumer -> producers)
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> dependency_map;
    for (auto& [producer, consumers] : dependency_graph_) {
        for (auto* consumer : consumers) {
            dependency_map[consumer].push_back(producer);
        }
    }

    /// Worker thread function
    auto worker_func = [&] {
        while (true) {
            RenderPass* task = nullptr;
            {
                /// Lock scope for condition variable wait
                std::unique_lock<std::mutex> lock(task_mutex);

                /// Wait until task available or all tasks completed
                cv.wait(lock, [&] {
                    /// Exit if no remaining tasks
                    if (ready_tasks.empty()) {
                        return true;
                    }

                    /// Find task with satisfied dependencies
                    for (auto it = ready_tasks.begin(); it != ready_tasks.end(); ++it) {
                        bool dependencies_met = true;
                        /// Check all producer dependencies
                        if (auto deps = dependency_map.find(*it); deps != dependency_map.end()) {
                            for (auto* dep : deps->second) {
                                if (!dep->IsCompleted()) {
                                    dependencies_met = false;
                                    break;
                                }
                            }
                        }

                        /// Task ready for execution
                        if (dependencies_met) {
                            task = *it;
                            ready_tasks.erase(it);
                            return true;
                        }
                    }
                    return false;  /// No ready tasks found
                    });

                /// Exit worker if all tasks processed
                if (ready_tasks.empty()) {
                    return;
                }
            }

            /// Execute task outside lock scope
            task->Execute();
            task->completed_.store(true);  // Atomic completion flag

            /// Update completion counter under lock
            {
                std::lock_guard<std::mutex> lock(task_mutex);
                completed_count++;
            }

            /// Notify all workers about state change
            cv.notify_all();
        }
    };

    /// Launch worker threads (up to hardware concurrency)
    unsigned int worker_count = std::min(
        static_cast<unsigned int>(execution_order_.size()),
        std::thread::hardware_concurrency()
    );
    for (unsigned int i = 0; i < worker_count; ++i) {
        workers.emplace_back(worker_func);
    }

    /// Wait for all workers to finish
    for (auto& worker : workers) {
        worker.join();
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