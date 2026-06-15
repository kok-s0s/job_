# 04 TCP Echo Server（多线程 Socket）

> 覆盖 JD 核心考点：Socket API、多线程、mutex、半关闭

## 项目结构

```
projects/tcp_echo_server/
├── CMakeLists.txt
└── src/
    ├── server.cpp   # 服务端
    └── client.cpp   # 客户端
```

## 编译与运行

```bash
cd projects/tcp_echo_server
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 终端 1：启动服务端
./build/server

# 终端 2/3：启动多个客户端
./build/client
```

## 设计要点

### 服务端：每客户端一线程 + detach

```cpp
std::thread(handle_client, client_fd, peer).detach();
```

`detach` 让每个客户端线程自己管理自己的生命周期。用 `join` 的话主线程就变串行了——必须等一个客户端断开才能 `accept` 下一个。

### send 循环

```cpp
while (sent < n) {
    int r = send(client_fd, buf + sent, n - sent, 0);
    if (r <= 0) goto done;
    sent += r;
}
```

`send` 不保证一次发完（内核缓冲区满时只发部分），必须循环补发。初学者最常见的 bug 之一。

### SO_REUSEADDR

```cpp
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

服务端重启时端口处于 `TIME_WAIT`，没有这个选项 `bind` 会报 "Address already in use"。

### mutex 保护 cout

```cpp
static std::mutex g_cout_mtx;
static void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_cout_mtx);
    std::cout << msg << "\n";
}
```

`std::cout` 不是线程安全的，多线程同时写会输出乱码。

### 客户端：shutdown(SHUT_WR) 半关闭

```cpp
shutdown(sock, SHUT_WR);   // 只关写端
recv_thread.join();         // 等 echo 全部收完
```

`SHUT_RDWR` 会同时关掉读端，echo 还没到就丢了。只关写端，服务端看到 EOF 关闭连接，客户端 recv 线程自然退出。

## 面试常问

**Q：为什么不用 `join` 而用 `detach`？**

`join` 要求主线程等待子线程结束，而 `accept` 循环需要立即处理下一个连接，两者互斥。`detach` 把线程的所有权交给运行时，客户端断开后线程自动回收。

**Q：`send` 返回值小于请求长度怎么办？**

循环补发，直到全部字节发出为止。TCP 发送缓冲区满时会出现这种情况。

**Q：`TIME_WAIT` 是什么？**

主动关闭连接的一方发出最后一个 ACK 后等待 2MSL（约 60 秒），防止最后的 ACK 丢失导致对方重传 FIN 时无人响应。`SO_REUSEADDR` 允许在 `TIME_WAIT` 期间重新绑定同一端口。

## 下一步

- **广播模式**：一个客户端发消息，所有客户端都收到（需要 `vector<int>` 客户端列表 + mutex）
- **线程池版**：固定 N 个 worker 线程 + 任务队列 + condition_variable
- **epoll 版**：单线程非阻塞，处理高并发
