# 2026-07-15 练习实现：线程池最小实现

对应练习：[2026-07-15：线程池最小实现](/roadmap/daily/2026-07-15)

源码文件：`practice/today_2026_07_15_thread_pool.cpp`

## 编译运行

```bash
c++ -std=c++17 -Wall -Wextra -pedantic practice/today_2026_07_15_thread_pool.cpp -o /tmp/today_thread_pool
/tmp/today_thread_pool
```

## 验证点

- `ThreadPool` 构造时创建 4 个 worker。
- `submit()` 把任务放入共享任务队列，并 `notify_one()`。
- worker 用 `condition_variable` 等待任务或停止信号。
- 析构时设置 `stopped_`，`notify_all()`，然后 `join()` 所有 worker。
- worker 只有在 `stopped_ && tasks_.empty()` 时退出，保证已提交任务不丢。
- 20 个任务全部完成，总和为 `2470`。

## 完整代码

```cpp
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count) {
        workers_.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            workers_.emplace_back([this, i] {
                workerLoop(i);
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                ++rejected_count_;
                return false;
            }
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
        return true;
    }

    int rejectedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return rejected_count_;
    }

private:
    void workerLoop(std::size_t worker_id) {
        (void)worker_id;

        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] {
                    return stopped_ || !tasks_.empty();
                });

                if (stopped_ && tasks_.empty()) {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_{false};
    int rejected_count_{0};
};

int main() {
    constexpr std::size_t worker_count = 4;
    constexpr int task_count = 20;

    std::vector<int> results(task_count, -1);
    std::mutex result_mutex;
    int submitted = 0;
    int completed = 0;

    int rejected = 0;
    {
        ThreadPool pool(worker_count);

        for (int i = 0; i < task_count; ++i) {
            const bool accepted = pool.submit([i, &results, &result_mutex, &completed] {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                const int value = i * i;
                {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    results[i] = value;
                    ++completed;
                }
            });

            if (accepted) {
                ++submitted;
            }
        }

        rejected = pool.rejectedCount();
    }

    int sum = 0;
    bool values_ok = true;
    for (int i = 0; i < task_count; ++i) {
        sum += results[i];
        values_ok = values_ok && results[i] == i * i;
    }

    const bool pass = submitted == task_count &&
                      completed == task_count &&
                      rejected == 0 &&
                      values_ok &&
                      sum == 2470;

    std::cout << "worker_count: " << worker_count << '\n';
    std::cout << "task_count: " << task_count << '\n';
    std::cout << "submitted: " << submitted << '\n';
    std::cout << "completed: " << completed << '\n';
    std::cout << "rejected: " << rejected << '\n';
    std::cout << "sum: " << sum << '\n';
    std::cout << "result: " << (pass ? "pass" : "fail") << '\n';

    return pass ? 0 : 1;
}
```
