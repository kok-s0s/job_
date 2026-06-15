# 06 TCP Chat Server（线程池版）

> 核心新增：ThreadPool 类——固定 N 个 worker + 任务队列 + condition_variable

## 与上一版的唯一差异

| | Chat Server（每连接一线程）| Chat Server（线程池）|
|--|--|--|
| 线程数 | 随连接数增长，无上限 | 固定 N 个 worker |
| 新连接处理 | `std::thread(...).detach()` | `pool.submit(task)` |
| 线程创建开销 | 每次连接都创建 | 启动时一次性创建 |
| 超出容量时 | 系统资源耗尽 | 任务排队等待空闲 worker |

## ThreadPool 实现（thread_pool.hpp）

```cpp
class ThreadPool {
public:
    explicit ThreadPool(std::size_t n_workers) {
        for (std::size_t i = 0; i < n_workers; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    }

    ~ThreadPool() {
        { std::lock_guard lock(mtx_); stop_ = true; }
        cv_.notify_all();          // 唤醒所有 worker，让它们退出
        for (auto& t : workers_) t.join();
    }

    void submit(std::function<void()> task) {
        { std::lock_guard lock(mtx_); tasks_.push(std::move(task)); }
        cv_.notify_one();          // 唤醒一个等待的 worker
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mtx_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();   // 在锁外执行任务，其他 worker 可以同时取任务
        }
    }

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mtx_;
    std::condition_variable           cv_;
    bool                              stop_{false};
};
```

## 三个关键细节

### unique_lock，不是 lock_guard

`cv_.wait()` 内部需要临时释放锁（挂起期间让其他线程能 submit），再重新加锁（被唤醒后）。`lock_guard` 不支持手动 unlock，只有 `unique_lock` 可以。

### 谓词防虚假唤醒

```cpp
cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
```

等价于：
```cpp
while (!stop_ && tasks_.empty())
    cv_.wait(lock);
```

线程可能在没有 `notify` 的情况下被唤醒（虚假唤醒，spurious wakeup，POSIX 允许），谓词确保条件不满足时继续睡。

### 析构时 notify_all

```cpp
cv_.notify_all();   // 不是 notify_one
```

`notify_one` 只唤醒一个，其他 worker 会永远卡在 `wait` 里，`join` 永远不会返回，析构函数死锁。

## 面试常问

**Q：线程池和每连接一线程，怎么选？**

连接数可预期且不大 → 每连接一线程更简单。高并发（C10K+）→ 线程池（或 epoll），避免线程创建/销毁开销和系统线程上限的约束。

**Q：`unique_lock` 和 `lock_guard` 的区别？**

`lock_guard` 构造时锁，析构时解锁，不能手动控制，开销低。`unique_lock` 支持 `unlock()`/`lock()` 手动控制，可以配合条件变量，稍有开销。

**Q：线程池的任务队列满了怎么办？**

本实现无上限（`std::queue` 可以无限增长）。生产级线程池通常加有界队列：满时 `submit` 阻塞或返回失败，避免内存无限增长。

**Q：为什么 worker 要在锁外执行 task？**

task 执行可能很慢（比如处理网络 I/O）。如果持锁执行，其他 worker 无法从队列取任务，线程池退化为串行。

## 下一步

- **epoll 版**：单线程非阻塞 I/O，真正的 C10K 方案
