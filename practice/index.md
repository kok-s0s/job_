# 动手实践

从 ROS2 入门到 C++ 并发、网络编程、IPC——按故事线递进，每个项目都能编译运行。

## 落地项目

<div class="pg">

<a class="pc" href="./robocon">
  <div class="pc-num">12</div>
  <div class="pc-title">RoboMon — 机械臂仿真控制台</div>
  <div class="pc-desc">把状态机 / 多线程 / BlockingQueue / Observer 全部组合，终端交互控制机械臂工作流，传感器实时回显</div>
  <div class="tags"><span class="tag">落地</span><span class="tag">state machine</span><span class="tag">多线程</span><span class="tag">ANSI</span></div>
</a>

</div>

<style>
.pg {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: 14px;
  margin-top: 24px;
}
.pc {
  border: 1px solid var(--vp-c-divider);
  border-radius: 10px;
  padding: 18px 20px;
  text-decoration: none !important;
  color: inherit;
  display: block;
  transition: border-color .2s, box-shadow .2s;
}
.pc:hover {
  border-color: var(--vp-c-brand-1);
  box-shadow: 0 2px 14px rgba(0,0,0,.08);
}
.pc-num  { font-size: 11px; color: var(--vp-c-text-3); margin-bottom: 6px; }
.pc-title{ font-size: 15px; font-weight: 600; color: var(--vp-c-text-1); margin-bottom: 8px; }
.pc-desc { font-size: 12px; color: var(--vp-c-text-2); line-height: 1.6; margin-bottom: 10px; }
.tags    { display: flex; flex-wrap: wrap; gap: 5px; }
.tag     { font-size: 11px; padding: 1px 7px; border-radius: 4px;
           background: var(--vp-c-default-soft); color: var(--vp-c-text-2); }
</style>

## ROS2 入门

<div class="pg">

<a class="pc" href="./pubsub_model">
  <div class="pc-num">01</div>
  <div class="pc-title">Pub/Sub 模型（Python）</div>
  <div class="pc-desc">用纯 Python 跑通发布/订阅核心机制，建立 ROS2 前置直觉</div>
  <div class="tags"><span class="tag">Python</span><span class="tag">pub/sub</span></div>
</a>

<a class="pc" href="./ros2_three_nodes">
  <div class="pc-num">02</div>
  <div class="pc-title">ROS2 三节点 Demo</div>
  <div class="pc-desc">Talker / Listener / Relay 三节点通信，CMake 工程结构</div>
  <div class="tags"><span class="tag">ROS2</span><span class="tag">topic</span><span class="tag">CMake</span></div>
</a>

<a class="pc" href="./ros2_service_action">
  <div class="pc-num">03</div>
  <div class="pc-title">Service & Action</div>
  <div class="pc-desc">同步请求/响应 vs 长时任务带反馈，两种通信模式对比</div>
  <div class="tags"><span class="tag">ROS2</span><span class="tag">service</span><span class="tag">action</span></div>
</a>

</div>

## 网络编程（TCP 进阶路线）

<div class="pg">

<a class="pc" href="./tcp_echo_server">
  <div class="pc-num">04</div>
  <div class="pc-title">TCP Echo Server</div>
  <div class="pc-desc">Socket API 完整流程，每客户端一线程，send 循环补发</div>
  <div class="tags"><span class="tag">socket</span><span class="tag">thread</span><span class="tag">mutex</span></div>
</a>

<a class="pc" href="./tcp_chat_server">
  <div class="pc-num">05</div>
  <div class="pc-title">TCP Chat Server（广播）</div>
  <div class="pc-desc">全局客户端列表 + broadcast，快照后释放锁再发送</div>
  <div class="tags"><span class="tag">broadcast</span><span class="tag">mutex</span><span class="tag">SIGPIPE</span></div>
</a>

<a class="pc" href="./tcp_chat_threadpool">
  <div class="pc-num">06</div>
  <div class="pc-title">TCP Chat Server（线程池）</div>
  <div class="pc-desc">固定 N 个 worker + 任务队列，condition_variable 调度</div>
  <div class="tags"><span class="tag">thread pool</span><span class="tag">condition_variable</span></div>
</a>

<a class="pc" href="./tcp_chat_epoll">
  <div class="pc-num">07</div>
  <div class="pc-title">TCP Chat Server（epoll）</div>
  <div class="pc-desc">单线程事件循环，epoll/kqueue 跨平台抽象，彻底无锁</div>
  <div class="tags"><span class="tag">epoll</span><span class="tag">kqueue</span><span class="tag">事件循环</span></div>
</a>

</div>

## OOP 设计模式

<div class="pg">

<a class="pc" href="./observer">
  <div class="pc-num">11</div>
  <div class="pc-title">Observer 模式</div>
  <div class="pc-desc">Subject / Observer 接口分离，状态机事件广播，Qt signals/slots 底层思路</div>
  <div class="tags"><span class="tag">OOP</span><span class="tag">virtual</span><span class="tag">template</span></div>
</a>

</div>

## C++ 并发 & 数据结构

<div class="pg">

<a class="pc" href="./state_machine">
  <div class="pc-num">08</div>
  <div class="pc-title">状态机（机械臂场景）</div>
  <div class="pc-desc">通用 StateMachine&lt;State,Event&gt; 模板，enum class，std::function 回调</div>
  <div class="tags"><span class="tag">template</span><span class="tag">enum class</span><span class="tag">std::function</span></div>
</a>

<a class="pc" href="./blocking_queue">
  <div class="pc-num">09</div>
  <div class="pc-title">线程安全队列（MPMC）</div>
  <div class="pc-desc">有界阻塞队列，两个 cv 精准唤醒，shutdown 优雅退出，背压演示</div>
  <div class="tags"><span class="tag">mutex</span><span class="tag">condition_variable</span><span class="tag">backpressure</span></div>
</a>

<a class="pc" href="./shm_ipc">
  <div class="pc-num">10</div>
  <div class="pc-title">共享内存 IPC</div>
  <div class="pc-desc">跨进程零拷贝传输，shm_open + 命名信号量，ring buffer 同步</div>
  <div class="tags"><span class="tag">shm_open</span><span class="tag">semaphore</span><span class="tag">IPC</span></div>
</a>

</div>
