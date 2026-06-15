# 10 共享内存 IPC

> 跨进程零拷贝数据传输：shm_open + 命名信号量 + ring buffer，两个独立进程共享 LiDAR 数据流

## 与线程安全队列的对比

| | BlockingQueue（09）| 共享内存 IPC（10）|
|--|--|--|
| 通信范围 | 同进程内多线程 | 跨进程 |
| 同步原语 | `std::mutex` + `condition_variable` | POSIX 命名信号量 |
| 数据拷贝 | 无（移动语义）| 无（mmap 共享物理页）|
| 内存类型 | 只能 POD + heap 分配 | 只能 **POD**（无堆指针）|

## 三个信号量的分工

```
SEM_MUTEX  (初始=1)  二值信号量，保护 ring buffer 的读写
SEM_FULL   (初始=0)  计数：当前可读的 slot 数，reader 在此等
SEM_EMPTY  (初始=N)  计数：当前空闲的 slot 数，writer 在此等
```

**为什么不用 sem_init？**
macOS 不支持 `sem_init` 的 `pshared=1`（进程间共享），命名信号量 `sem_open` 在 Linux 和 macOS 上都能用。

## writer 操作序列

```cpp
sem_wait(empty);    // 等有空位
sem_wait(mutex);    // 加锁
shm->frames[shm->head] = frame;
shm->head = (shm->head + 1) % CAPACITY;
sem_post(mutex);    // 解锁
sem_post(full);     // 通知 reader 有新数据
```

## reader 操作序列

```cpp
sem_wait(full);     // 等有数据
sem_wait(mutex);    // 加锁
frame = shm->frames[shm->tail];
shm->tail = (shm->tail + 1) % CAPACITY;
sem_post(mutex);    // 解锁
sem_post(empty);    // 通知 writer 有空位
```

两个方向完全对称，和线程安全队列里 `not_full_`/`not_empty_` 的逻辑一模一样，只是换了 API。

## 共享内存生命周期

```cpp
// writer：创建
shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666)
ftruncate(fd, sizeof(SharedRegion))
mmap(...)

// reader：附着（不带 O_CREAT）
shm_open(SHM_NAME, O_RDWR, 0666)
mmap(...)

// reader 负责最终清理（确保 writer 先退出）
shm_unlink(SHM_NAME)
sem_unlink(SEM_MUTEX / SEM_FULL / SEM_EMPTY)
```

`shm_unlink` 后名字从文件系统消失，但已 `mmap` 的进程仍可继续访问（引用计数降到 0 才真正释放）。

## 共享内存只能放 POD

```cpp
// ✅ 合法
struct LidarFrame { int id; float dist; };

// ❌ 非法：std::string 内部有堆指针，在另一个进程地址空间里无效
struct Bad { std::string name; };
```

## 面试常问

**Q：共享内存和管道（pipe）的区别？**

管道是字节流，需要拷贝数据，有内核缓冲区。共享内存直接映射到两个进程的地址空间，读写即内存访问，零拷贝，延迟最低。代价是需要自己做同步（信号量/mutex）。

**Q：sem_wait / sem_post 是原子的吗？**

是。sem_wait 原子地将值减 1（值为 0 时阻塞），sem_post 原子地将值加 1 并唤醒等待者，由内核保证。

**Q：如果 writer 崩溃，信号量和共享内存会泄漏吗？**

会。POSIX 命名信号量和共享内存在进程退出后仍然存在（除非显式 unlink）。实际系统里需要在启动时做清理（`sem_unlink` + `shm_unlink`），或者用信号处理捕获 SIGTERM/SIGABRT 做善后。本项目 writer 启动时就 unlink 了旧的信号量，即是这个思路。

**Q：ROS2 里用共享内存吗？**

ROS2 的 `rmw_fastrtps`（默认中间件）支持 intra-process 零拷贝，底层原理类似：同节点内的 publisher/subscriber 直接共享指针。跨节点或跨机器则走 DDS/UDP。
