#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class ThreadPool
 * @brief Manages a pool of worker threads for concurrent task execution
 */
class ThreadPool {
public:
	/**
	 * @brief Constructs thread pool with specified worker count
	 * @param threads Number of worker threads (default = hardware concurrency)
	 */
	explicit ThreadPool(unsigned int threads = std::thread::hardware_concurrency());

	/// Stops all threads and cleans up resources
	~ThreadPool();

	/**
	 * @brief Enqueues a task for asynchronous execution
	 * @tparam F Callable type (function/lambda)
	 * @param f Task to execute
	 */
	template<class F>
	inline void Enqueue(F&& f) {
		{
			// Lock task queue for thread-safe modification
			std::unique_lock<std::mutex> lock(queue_mutex_);

			// Add task to queue via perfect forwarding
			tasks_.emplace(std::forward<F>(f));
		}

		// Notify one waiting worker thread
		condition_.notify_one();
	}

protected:
	std::vector<std::thread> workers_;         ///< Worker thread collection
	std::queue<std::function<void()>> tasks_;  ///< FIFO task queue
	std::mutex queue_mutex_;                   ///< Protects task queue access
	std::condition_variable condition_;        ///< Coordinates task notification
    bool stop_;                                ///< Termination flag
};

NAMESPACE_END(dream)