# 07 TCP Chat Server（epoll / kqueue 版）

> 核心转变：从「多线程」到「单线程事件循环」，彻底不需要 mutex

## 三版服务端对比

| | 每连接一线程 | 线程池 | epoll 单线程 |
|--|--|--|--|
| 线程数 | = 连接数 | 固定 N | **1** |
| 共享状态保护 | mutex | mutex | **不需要** |
| 10k 连接开销 | 10k 线程 | N 线程 + 大队列 | **1 线程** |
| 代码复杂度 | 低 | 中 | 中（事件驱动思维）|

## 核心思路

```
while (true) {
    poller.wait(ready_fds)    // 阻塞，直到有 fd 可读
    for fd in ready_fds:
        if fd == server_fd → accept 新客户端，注册进 poller
        else               → recv 数据，broadcast 或处理断线
}
```

所有操作都在同一个线程内串行执行，`g_clients` 不会被并发访问，**不需要任何锁**。

## 平台抽象（poller.hpp）

epoll（Linux）和 kqueue（macOS）API 不同，但语义相同：注册 fd → 等待可读事件 → 拿到就绪 fd 列表。

```cpp
// Linux: epoll
void add(int fd) {
    epoll_event ev{ .events = EPOLLIN, .data = {.fd = fd} };
    epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}
void wait(vector<int>& ready) {
    int n = epoll_wait(epfd_, events, MAX, -1);
    for (int i = 0; i < n; i++) ready.push_back(events[i].data.fd);
}

// macOS: kqueue（等价语义）
void add(int fd) {
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    kevent(kqfd_, &ev, 1, nullptr, 0, nullptr);
}
void wait(vector<int>& ready) {
    int n = kevent(kqfd_, nullptr, 0, events, MAX, nullptr);
    for (int i = 0; i < n; i++) ready.push_back(events[i].ident);
}
```

## 非阻塞 socket 与 EAGAIN

```cpp
int n = recv(fd, buf, sizeof(buf), 0);
if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return; // 暂无数据，正常
```

所有 socket 设为非阻塞（`O_NONBLOCK`）。`recv` 没有数据时不挂起，而是返回 `-1` + `errno=EAGAIN`。事件循环收到这个信号说明"这次读完了，等下次 epoll 通知"。

## 客户端断线处理

```cpp
std::string peer = g_clients[fd];
poller.remove(fd);
g_clients.erase(fd);   // 先从 map 删除
close(fd);
broadcast(-1, msg);    // fd 已不在 map 里，-1 相当于"发给所有人"
```

先 erase 再 broadcast，避免向已关闭的 fd 发送数据。

## 面试常问

**Q：epoll 为什么比 select/poll 快？**

`select`/`poll` 每次调用都要把全部监听 fd 从用户态拷贝到内核态，O(n) 扫描。`epoll` 用红黑树维护注册集合，用链表维护就绪集合，`epoll_wait` 只返回真正就绪的 fd，O(1)。

**Q：水平触发（LT）和边缘触发（ET）的区别？**

- **LT（默认）**：fd 可读时，每次 `epoll_wait` 都会通知，直到数据被读完。
- **ET**：fd 变为可读时只通知一次，必须循环 `recv` 直到 `EAGAIN`，否则漏数据。

ET 效率更高但编程更复杂，Nginx 用 ET。本项目用 LT，更容易写正确。

**Q：单线程事件循环的瓶颈是什么？**

CPU 密集型任务会阻塞事件循环，导致其他连接无法响应。解决方案：事件循环只做 I/O，CPU 密集任务 offload 到线程池（epoll + 线程池组合，即 Reactor 模式）。

**Q：epoll 是 Linux 专有的，跨平台怎么办？**

macOS/BSD 用 kqueue，Windows 用 IOCP。可以用 `#ifdef` 屏蔽差异（本项目做法），或使用 libuv、Boost.Asio 等跨平台库。

## 学习路径回顾

```
Echo Server（单客户端阻塞）
    ↓ 每连接一线程
Chat Server（多线程广播）
    ↓ 固定线程数 + 任务队列
Chat Server（线程池 + condition_variable）
    ↓ 单线程 + I/O 多路复用
Chat Server（epoll/kqueue 事件循环）← 当前
    ↓
Reactor 模式（epoll + 线程池组合）← 工业级
```
