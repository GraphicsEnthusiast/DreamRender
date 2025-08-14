#include <render_graph.h>

NAMESPACE_BEGIN(dream)

RenderGraph::~RenderGraph() {
    // Ensure all sync objects are cleaned up
    CleanupSyncObjects();
}

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
    // Cleanup previous frame's sync objects
    CleanupSyncObjects();

    std::mutex task_mutex;
    std::condition_variable cv;
    std::atomic<bool> all_tasks_completed = false;

    std::mutex done_mutex;
    std::condition_variable done_cv;

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
            std::shared_ptr<RenderContext> context;

            {
                std::unique_lock<std::mutex> lock(task_mutex);

                // Wait for ready task or completion signal
                cv.wait(lock, [&] {
                    if (ready_tasks.empty() || all_tasks_completed) {
                        return true;
                    }

                    // Find task with satisfied dependencies
                    for (auto it = ready_tasks.begin(); ready_tasks.end() != it; ++it) {
                        bool deps_met = true;

                        // Check all producer dependencies
                        if (auto deps = reverse_deps.find(*it); reverse_deps.end() != deps) {
                            for (auto* dep : deps->second) {
                                if (!dep->IsCompleted(std::memory_order_acquire)) {
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
                auto UpdateCompletionCounter = [&]() {
                    {
                        std::unique_lock<std::mutex> lock(task_mutex);
                        completed_count++;
                        if (completed_count == total_tasks) {
                            done_cv.notify_one();
                        }
                    }
                    cv.notify_all();
                };

                try {
                    // 1. Activate pass - specific context if available
                    if (task->context_) {
                        task->context_->MakeCurrent();
                    }

                    // 2. Check OpenGL errors before execution
                    GLenum pre_err = glGetError();
                    if (GL_NO_ERROR != pre_err) {
                        WARN("[warning] Pre-execution OpenGL error: 0x%X.", pre_err);
                    }

                    // 3. Wait for producer dependencies
                    if (auto deps = reverse_deps.find(task); deps != reverse_deps.end()) {
                        for (auto* dep_pass : deps->second) {
                            if (GLsync sync = dep_pass->GetSync()) {
                                // Check context compatibility
                                bool safe_to_wait = false;

                                // Case 1: Same context
                                if (task->context_ == dep_pass->context_) {
                                    safe_to_wait = true;
                                }
                                // Case 2: Sharing contexts
                                else if (task->context_ && dep_pass->context_ &&
                                    task->context_->IsSharingWith(dep_pass->context_.get())) {
                                    safe_to_wait = true;
                                }
                                // Case 3: Main context and offscreen
                                else if (!task->context_ && dep_pass->context_ &&
                                    dep_pass->context_->IsSharingWith(main_window_)) {
                                    safe_to_wait = true;
                                }

                                if (safe_to_wait) {
                                    glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                                }
                                else {
                                    WARN("[warning] Skipping sync wait due to context incompatibility.");
                                }
                            }
                        }
                    }

                    // 4. Execute the pass
                    task->Execute();

                    // 5. Check OpenGL errors after execution
                    GLenum err = glGetError();
                    while (GL_NO_ERROR != err) {
                        std::string error_str;
                        switch (err) {
                        case GL_INVALID_ENUM: error_str = "GL_INVALID_ENUM"; break;
                        case GL_INVALID_VALUE: error_str = "GL_INVALID_VALUE"; break;
                        case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION"; break;
                        case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                        case GL_OUT_OF_MEMORY: error_str = "GL_OUT_OF_MEMORY"; break;
                        default: error_str = "Unknown error"; break;
                        }
                        ERROR("[error] OpenGL error during pass execution: %s.", error_str.c_str());
                        err = glGetError();
                    }

                    // 6. Create new sync fence
                    GLsync new_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                    task->SetSync(new_sync);
                    {
                        std::lock_guard<std::mutex> lock(sync_mutex_);
                        frame_sync_objects_.push_back(new_sync);
                    }

                    // 7. Update completion counter
                    task->SetCompleted(std::memory_order_release);

                    // 8. Release context after execution
                    if (task->context_) {
                        task->context_->Release();
                    }

                    // 9. Update completion counter
                    UpdateCompletionCounter();
                }
                catch (const std::exception& e) {
                    ERROR("[error] Exception in render pass: %s.", e.what());

                    task->SetCompleted(std::memory_order_release);

                    UpdateCompletionCounter();
                }
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
    {
        std::unique_lock<std::mutex> lock(done_mutex);
        done_cv.wait(lock, [&] {
            return completed_count == total_tasks;
            });
    }

    // Signal threads to exit
    {
        std::unique_lock<std::mutex> lock(task_mutex);
        all_tasks_completed = true;
    }
    cv.notify_all();
}

TextureHandle RenderGraph::GetFinalOutput() const noexcept {
    return final_output_;
}

void RenderGraph::AssignContextToPass(const std::string& pass_name, unsigned int width, unsigned int height) {
    auto context = std::make_shared<RenderContext>(main_window_, width, height);
    pass_contexts_[pass_name] = context;

    if (auto pass = GetPass(pass_name)) {
        pass->SetContext(context);
    }
}

void RenderGraph::CleanupSyncObjects() {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    for (auto it = frame_sync_objects_.begin(); it != frame_sync_objects_.end();) {
        GLsync sync = *it;
        if (sync) {
            GLint status = GL_UNSIGNALED;
            glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), nullptr, &status);

            if (GL_SIGNALED == status) {
                glDeleteSync(sync);
            }
            else {
                // Add a timeout mechanism to avoid permanent blocking.
                GLenum wait = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); // 1 s timeout
                if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED) {
                    glDeleteSync(sync);
                }
                else {
                    ERROR("[error] Sync object wait timeout, force delete.");
                    glDeleteSync(sync);
                }
            }
            it = frame_sync_objects_.erase(it);
        }
        else {
            ++it;
        }
    }
}

RenderPass* RenderGraph::GetPass(const std::string& name) {
    if (auto it = passes_.find(name); passes_.end() != it) {
        return it->second.get();
    }

    return nullptr;
}

NAMESPACE_END(dream)