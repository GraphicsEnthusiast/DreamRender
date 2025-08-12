#pragma once

#include <utils.h>

class ThreadPool {
public:
    explicit ThreadPool(unsigned int threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<class F>
    inline void Enqueue(F&& f) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace(std::forward<F>(f));

        condition_.notify_one();
    }

protected:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};