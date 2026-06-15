# 05 TCP Chat Server（广播模式）

> 在 Echo Server 基础上新增：全局客户端列表 + broadcast，引入共享状态的并发保护

## 项目结构

```
projects/tcp_chat_server/
├── CMakeLists.txt
└── src/
    ├── server.cpp
    └── client.cpp   # 支持 ./client <昵称>
```

## 编译与运行

```bash
cd projects/tcp_chat_server
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 终端 1：启动服务端
./build/server

# 终端 2/3/4：各自连入
./build/client alice
./build/client bob
./build/client carol
```

## 与 Echo Server 的核心差异

| | Echo Server | Chat Server |
|--|--|--|
| 收消息的人 | 发送者自己 | 所有其他客户端 |
| 共享状态 | 无 | `g_clients`（全局客户端列表）|
| 新增保护 | 无 | `g_clients_mtx` |

## 设计要点

### 全局客户端注册表

```cpp
static std::unordered_map<int, std::string> g_clients;  // fd → "ip:port"
static std::mutex                            g_clients_mtx;
```

客户端连入时 `insert`，断开时 `erase`，两个操作都要加锁。

### broadcast：先快照，再发送

```cpp
void broadcast(int sender_fd, const std::string& msg) {
    std::vector<int> recipients;
    {
        std::lock_guard<std::mutex> lock(g_clients_mtx);
        for (auto& [fd, _] : g_clients)
            if (fd != sender_fd) recipients.push_back(fd);
    }  // ← 锁在这里释放

    for (int fd : recipients)
        send(fd, msg.data(), msg.size(), 0);
}
```

**为什么持锁期间不能直接 send？**  
`send` 在对端缓冲区满时会阻塞。如果某个客户端卡住，锁就一直被占着，所有其他线程想修改 `g_clients`（比如新客户端 join / 某人 leave）全部排队等待。拷贝一份快照，锁的粒度只是遍历 map 的那一瞬间。

### SIGPIPE 处理

```cpp
signal(SIGPIPE, SIG_IGN);
```

向已关闭的 socket 发数据，OS 默认会给进程发 `SIGPIPE` 信号杀死进程。忽略它后，`send` 改为返回 `-1` + `errno=EPIPE`，服务端可以继续运行。

## 实际输出示例

```
=== alice 的视角 ===
[*] connected as alice
*** 127.0.0.1:52442 joined ***   ← bob 进来了
*** 127.0.0.1:52444 joined ***   ← carol 进来了
[127.0.0.1:52442] bob: hey alice!
[127.0.0.1:52444] carol: hello~
*** 127.0.0.1:52444 left ***
*** 127.0.0.1:52442 left ***

=== carol 的视角 ===
[*] connected as carol
[127.0.0.1:52443] alice: hi everyone
[127.0.0.1:52442] bob: hey alice!
```

## 面试常问

**Q：为什么要复制快照而不是持锁发送？**

持锁调用 `send` 会把锁的粒度扩大到网络 I/O，慢客户端会让所有线程卡住。快照后释放锁，把网络 I/O 放到锁外，是常见的"减小锁粒度"技巧。

**Q：快照方案有什么代价？**

快照复制完到真正发出这段时间内，某个 `fd` 可能已经 `close` 了，`send` 会返回错误。需要在发送失败时忽略或处理这个 `fd`，而不是 crash。本实现依赖 `SIGPIPE SIG_IGN` 让 `send` 返回 `-1` 而非终止进程。

**Q：`unordered_map` 和 `vector` 这里用哪个更合适？**

`unordered_map<fd, name>` 方便按 fd 快速查找（O(1) erase）；如果只需要遍历，`unordered_set<fd>` 也可以。`vector` 的 erase 是 O(n)，不适合频繁断线的场景。

## 下一步

- **线程池版**：固定 N 个 worker 线程 + 任务队列 + `condition_variable`，不再每连接一线程
- **epoll 版**：单线程非阻塞，IO 多路复用
