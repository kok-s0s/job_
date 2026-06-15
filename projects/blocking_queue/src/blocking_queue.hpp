#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

// Bounded, thread-safe MPMC (multiple-producer multiple-consumer) queue.
// push() blocks when full; pop() blocks when empty.
// shutdown() drains waiters so all threads can exit cleanly.
template<typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity) : capacity_(capacity) {}

    // Push an item. Blocks if the queue is full.
    // Returns false (without pushing) if the queue has been shut down.
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mtx_);
        not_full_.wait(lock, [this] { return closed_ || q_.size() < capacity_; });
        if (closed_) return false;
        q_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // Non-blocking push. Returns false immediately if full or shut down.
    bool try_push(T item) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (closed_ || q_.size() >= capacity_) return false;
        q_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // Pop an item. Blocks if the queue is empty.
    // Returns false only when the queue is shut down AND empty (fully drained).
    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [this] { return closed_ || !q_.empty(); });
        if (q_.empty()) return false;   // shut down + drained
        out = std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return true;
    }

    // Non-blocking pop. Returns false immediately if empty.
    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return true;
    }

    // Signal shutdown. Wakes all blocked push/pop callers.
    // Consumers can still drain remaining items; push() will fail immediately.
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            closed_ = true;
        }
        not_full_.notify_all();   // wake blocked producers → they return false
        not_empty_.notify_all();  // wake blocked consumers → they drain then return false
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return q_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return q_.empty();
    }

    std::size_t capacity() const { return capacity_; }
    bool        closed()   const {
        std::lock_guard<std::mutex> lock(mtx_);
        return closed_;
    }

private:
    const std::size_t        capacity_;
    std::queue<T>            q_;
    mutable std::mutex       mtx_;
    std::condition_variable  not_full_;
    std::condition_variable  not_empty_;
    bool                     closed_{false};
};
