#include <thread_pool.h>

NAMESPACE_BEGIN(dream)

std::unique_ptr<ThreadPool> ThreadPool::instance_ = nullptr;

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

ThreadPool& ThreadPool::Instance(unsigned int threads) {
	static std::once_flag initFlag;
	std::call_once(initFlag, [threads]() {
		instance_ = std::unique_ptr<ThreadPool>(new ThreadPool(threads));
		});

	return *instance_;
}

void ThreadPool::Release() {
	if (instance_) {
		instance_->ReleaseInstance();
		instance_.reset();
	}
}

void ThreadPool::ReleaseInstance() {
	{
		std::unique_lock<std::mutex> lock(queue_mutex_);
		stop_ = true;
	}
	condition_.notify_all();
	for (auto& worker : workers_) {
		if (worker.joinable()) worker.join();
	}
	workers_.clear();
	tasks_ = {};
}

NAMESPACE_END(dream)