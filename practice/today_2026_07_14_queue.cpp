#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class BlockingQueue {
public:
    BlockingQueue() = default;
    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    bool push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                ++dropped_after_stop_;
                return false;
            }
            queue_.push(value);
        }
        cv_.notify_one();
        return true;
    }

    bool pop(int& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return stopped_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return false;
        }

        value = queue_.front();
        queue_.pop();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    int droppedAfterStop() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return dropped_after_stop_;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<int> queue_;
    bool stopped_{false};
    int dropped_after_stop_{0};
};

int main() {
    BlockingQueue queue;

    constexpr int message_count = 1000;
    int produced_count = 0;
    int consumed_count = 0;
    long long sum = 0;

    std::thread consumer([&] {
        int value = 0;
        while (queue.pop(value)) {
            ++consumed_count;
            sum += value;
        }
    });

    std::thread producer([&] {
        for (int i = 0; i < message_count; ++i) {
            if (queue.push(i)) {
                ++produced_count;
            }
        }
        queue.shutdown();
    });

    producer.join();
    consumer.join();

    const long long expected_sum = 1LL * (message_count - 1) * message_count / 2;
    const bool pass = produced_count == message_count &&
                      consumed_count == message_count &&
                      sum == expected_sum &&
                      queue.size() == 0 &&
                      queue.droppedAfterStop() == 0;

    std::cout << "produced: " << produced_count << '\n';
    std::cout << "consumed: " << consumed_count << '\n';
    std::cout << "sum: " << sum << '\n';
    std::cout << "queue_size: " << queue.size() << '\n';
    std::cout << "dropped_after_stop: " << queue.droppedAfterStop() << '\n';
    std::cout << "result: " << (pass ? "pass" : "fail") << '\n';

    return pass ? 0 : 1;
}
