# 09 线程安全有界队列（MPMC）

> 练习点：两个 condition_variable 协作、shutdown 优雅退出、背压（backpressure）机制

## 场景

2 个传感器线程（producer）以 30ms/帧生产 LiDAR 数据，2 个处理线程（consumer）以 60ms/帧消费——生产快于消费，队列满后 producer 自动阻塞。

## 核心数据结构

```
BlockingQueue<T>
├── std::queue<T>           内部存储
├── std::mutex mtx_         保护队列
├── condition_variable not_full_    push() 在满时等这个
├── condition_variable not_empty_   pop()  在空时等这个
└── bool closed_            shutdown 标志
```

**为什么需要两个 condition_variable？**

`push` 关心的条件是"队列不满"，`pop` 关心的是"队列不空"，用两个 cv 可以精准唤醒：push 成功后只唤醒 consumer，pop 成功后只唤醒 producer，避免所有线程都被惊醒再重新竞争。

## 关键实现

### push — 满时阻塞

```cpp
bool push(T item) {
    std::unique_lock<std::mutex> lock(mtx_);
    not_full_.wait(lock, [this] { return closed_ || q_.size() < capacity_; });
    if (closed_) return false;
    q_.push(std::move(item));
    not_empty_.notify_one();   // 唤醒一个等待的 consumer
    return true;
}
```

### pop — 空时阻塞，shutdown 后仍可排干

```cpp
bool pop(T& out) {
    std::unique_lock<std::mutex> lock(mtx_);
    not_empty_.wait(lock, [this] { return closed_ || !q_.empty(); });
    if (q_.empty()) return false;   // shutdown + 已排干 → 通知调用方退出
    out = std::move(q_.front());
    q_.pop();
    not_full_.notify_one();    // 唤醒一个等待的 producer
    return true;
}
```

### shutdown — 两个 cv 都要 notify_all

```cpp
void shutdown() {
    { std::lock_guard lock(mtx_); closed_ = true; }
    not_full_.notify_all();    // 唤醒所有阻塞的 producer → 它们返回 false
    not_empty_.notify_all();   // 唤醒所有阻塞的 consumer → 它们排干剩余项
}
```

`notify_all` 不是 `notify_one`：shutdown 时要让 **所有** 等待线程醒来并退出，漏掉任何一个会导致 `join()` 永久挂起。

## 背压效果（实际输出片段）

```
[producer 0] push frame 3  (queue=4/4)  ← 满了，下次 push 会阻塞
[consumer 0] processed frame 102        ← consumer 消费一个，腾出空位
[producer 1] push frame 104  (queue=3/4) ← producer 被唤醒，继续 push
```

## 生产者/消费者退出顺序

```cpp
// 等所有 producer 完成
for (int i = NUM_CONSUMERS; i < threads.size(); ++i)
    threads[i].join();

q.shutdown();   // 通知 consumer 没有新数据了

// consumer 会排干队列中剩余项，然后 pop() 返回 false 退出
for (int i = 0; i < NUM_CONSUMERS; ++i)
    threads[i].join();
```

先 join producer 再 shutdown，确保不会在还有数据在路上时就告诉 consumer 退出。

## 面试常问

**Q：为什么 `size()` 和 `empty()` 也要加锁？**

即使只读，不加锁读 `q_.size()` 在另一个线程正在写时是未定义行为（UB）。同时 `mutable mutex` 允许在 `const` 方法里加锁。

**Q：`try_push` / `try_pop` 的用途？**

非阻塞版本适合"尽力而为"场景：超时重试、带 deadline 的任务、不想让线程阻塞的实时系统。阻塞版适合吞吐优先、线程可以等的场景。

**Q：有界 vs 无界队列怎么选？**

有界队列（本实现）：提供背压，防止内存无限增长，适合生产速率可能超过消费速率的场景（机器人传感器数据流）。无界队列：实现简单，但内存无上限，生产远快于消费时会 OOM。

**Q：这个实现是 MPMC 吗？**

是。`push`/`pop` 都加了 mutex，任意多个 producer 和 consumer 都可以安全并发访问。如果只需要 SPSC（单生产者单消费者），可以用无锁的 ring buffer（`std::atomic` + 内存序），性能更高。
