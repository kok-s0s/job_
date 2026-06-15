#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t n_workers) {
        workers_.reserve(n_workers);
        for (std::size_t i = 0; i < n_workers; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }
        // notify_all, not notify_one: every sleeping worker must wake and exit
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }

    // non-copyable, non-movable (threads hold a pointer to *this)
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                // unique_lock required: cv_.wait() needs to unlock/relock
                std::unique_lock<std::mutex> lock(mtx_);
                // predicate guards against spurious wakeups:
                // worker goes back to sleep if neither stop nor tasks
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();  // run outside the lock so other workers can pick tasks
        }
    }

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mtx_;
    std::condition_variable           cv_;
    bool                              stop_{false};
};
