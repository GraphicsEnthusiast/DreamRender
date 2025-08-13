#include <thread_pool.h>

NAMESPACE_BEGIN(dream)

ThreadPool::ThreadPool(unsigned int threads) : stop_(false) {
    // Create specified number of worker threads
    for (unsigned int i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            // Continuous task processing loop
            while (true) {
                std::function<void()> task;

                {
                    // Acquire lock for condition variable
                    std::unique_lock<std::mutex> lock(queue_mutex_);

                    // Wait until tasks available or stop signaled
                    condition_.wait(lock, [this] {
                        return stop_ || !tasks_.empty();
                        });

                    // Exit thread if stop requested and queue empty
                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    // Extract task from queue
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                // Execute task outside lock scope
                task();
            }
            });
    }
}

ThreadPool::~ThreadPool() {
    // Lock before modifying shared state
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Set termination flag
    stop_ = true;

    // Wake all waiting threads
    condition_.notify_all();

    // Join all worker threads
    for (std::thread& worker : workers_) {
        worker.join();
    }
}

NAMESPACE_END(dream)