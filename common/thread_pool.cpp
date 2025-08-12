#include <thread_pool.h>

ThreadPool::ThreadPool(unsigned int threads) : stop_(false) {
    for (unsigned int i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                    });
                if (stop_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();

                task();
            }
            });
    }
}

ThreadPool::~ThreadPool() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;

    condition_.notify_all();
    for(std::thread &worker: workers_) {
        worker.join();
    }  
}
