#pragma once

#include <utils.h>
#include <mutex>
#include <memory>

NAMESPACE_BEGIN(dream)

/**
 * @class ThreadPool
 * @brief Manages a pool of worker threads for concurrent task execution (Singleton)
 */
class ThreadPool {
public:
    /**
     * @brief Get the singleton instance
     * @param threads Number of worker threads (default = hardware concurrency)
     * @return ThreadPool& Reference to the singleton instance
     */
    static ThreadPool& Instance(unsigned int threads = std::thread::hardware_concurrency());

    /**
     * @brief Releases all resources and destroys the singleton instance
     */
    static void Release();

    /**
     * @brief Enqueues a task for asynchronous execution
     * @tparam F Callable type (function/lambda)
     * @param f Task to execute
     */
    template<class F>
    void Enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(f));
        }
        condition_.notify_one();
    }

    /**
     * @brief Prohibit copying instances
     */
    ThreadPool(const ThreadPool&) = delete;

    /**
     * @brief Prohibit assignment operations
     */
    ThreadPool& operator=(const ThreadPool&) = delete;

protected:
    /**
     * @brief Constructs a thread pool with specified worker count
     * @param threads Number of worker threads (default = hardware concurrency)
     */
    explicit ThreadPool(unsigned int threads = std::thread::hardware_concurrency());

    /**
     * @brief Internal resource cleanup method
     */
    void ReleaseInstance();

    std::vector<std::thread> workers_;           ///< Collection of worker threads
    std::queue<std::function<void()>> tasks_;    ///< FIFO queue for pending tasks
    std::mutex queue_mutex_;                     ///< Mutex for thread-safe queue access
    std::condition_variable condition_;          ///< Condition variable for task notification
    bool stop_;                                  ///< Termination flag (true to stop pool)
    static std::unique_ptr<ThreadPool> instance_;///< Singleton instance pointer
};

NAMESPACE_END(dream)