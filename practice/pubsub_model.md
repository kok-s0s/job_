# Pub/Sub 模型：从 Python 到 ROS2

> 在 ROS2 装好之前，用纯 Python 把发布/订阅的核心机制跑通，建立直觉。

## 系统结构

三个节点并发运行，通过两个 Topic 传递消息：

```mermaid
graph LR
    PUB["📡 imu_publisher<br/>(Publisher Node)<br/>50 Hz 发布 IMU 数据"]
    PROC["⚙️ processor<br/>(Sub + Pub Node)<br/>检测加速度异常"]
    ALERT["🔔 alert_listener<br/>(Subscriber Node)<br/>打印告警"]

    PUB -- "/imu/data<br/>ImuMsg" --> PROC
    PROC -- "/alerts<br/>AlertMsg" --> ALERT

    style PUB   fill:#1e3a5f,color:#fff,stroke:#4a90d9
    style PROC  fill:#1e3a5f,color:#fff,stroke:#4a90d9
    style ALERT fill:#1e3a5f,color:#fff,stroke:#4a90d9
```

**数据流**：IMU 传感器每秒 50 帧 → 处理节点判断加速度是否超阈值 → 超了就发告警。

---

## 可运行示例

将以下代码保存为 `pubsub_demo.py`，直接 `python3 pubsub_demo.py` 运行：

```python
import asyncio
import numpy as np
from dataclasses import dataclass, field
from time import monotonic

# ── 消息类型（对应 ROS2 里的 .msg 定义）─────────────────────────
@dataclass
class ImuMsg:
    stamp: float
    accel: np.ndarray = field(default_factory=lambda: np.zeros(3))
    gyro:  np.ndarray = field(default_factory=lambda: np.zeros(3))

@dataclass
class AlertMsg:
    stamp: float
    text: str

# ── Topic（带广播能力的异步队列）────────────────────────────────
class Topic:
    def __init__(self, name: str):
        self.name = name
        self._subscribers: list[asyncio.Queue] = []

    def subscribe(self) -> asyncio.Queue:
        q = asyncio.Queue(maxsize=20)       # QoS: 最多缓存 20 条
        self._subscribers.append(q)
        return q

    async def publish(self, msg):
        for q in self._subscribers:
            if not q.full():                # 满了就丢，对应 KEEP_LAST 策略
                await q.put(msg)

# ── 节点 1：IMU 发布者 ──────────────────────────────────────────
async def imu_publisher(topic: Topic, hz: int = 50):
    interval = 1.0 / hz
    count = 0
    while count < 100:                      # 发 100 帧退出
        msg = ImuMsg(
            stamp=monotonic(),
            accel=np.array([0.0, 0.0, 9.8]) + np.random.randn(3) * 0.1,
            gyro=np.random.randn(3) * 0.05,
        )
        await topic.publish(msg)
        await asyncio.sleep(interval)
        count += 1
    await topic.publish(None)               # None 是退出哨兵

# ── 节点 2：处理节点（订阅 IMU，发布告警）──────────────────────
async def processor(imu_topic: Topic, alert_topic: Topic):
    imu_queue = imu_topic.subscribe()
    threshold = 9.9                         # 加速度异常阈值 (m/s²)
    received = 0
    while True:
        msg: ImuMsg = await imu_queue.get()
        if msg is None:
            await alert_topic.publish(None)
            break
        received += 1
        norm = np.linalg.norm(msg.accel)
        if norm > threshold:
            await alert_topic.publish(
                AlertMsg(stamp=msg.stamp, text=f"|accel|={norm:.2f} 超阈值!")
            )
    print(f"[processor] 共处理 {received} 帧")

# ── 节点 3：告警订阅者 ──────────────────────────────────────────
async def alert_listener(alert_topic: Topic):
    queue = alert_topic.subscribe()
    count = 0
    while True:
        msg: AlertMsg = await queue.get()
        if msg is None:
            break
        count += 1
        print(f"  ⚠  t={msg.stamp:.3f}s  {msg.text}")
    print(f"[alert_listener] 共收到 {count} 条告警")

# ── 启动：三个节点并发运行 ──────────────────────────────────────
async def main():
    imu_topic   = Topic("/imu/data")
    alert_topic = Topic("/alerts")
    print("=== 系统启动 ===\n")
    await asyncio.gather(
        imu_publisher(imu_topic, hz=50),
        processor(imu_topic, alert_topic),
        alert_listener(alert_topic),
    )
    print("\n=== 所有节点退出 ===")

asyncio.run(main())
```

---

## Python 与 ROS2 API 的对照

```mermaid
graph TB
    subgraph Python["Python（本示例）"]
        direction TB
        T1["Topic('/imu/data')"]
        T2["asyncio.Queue(maxsize=20)"]
        T3["await topic.publish(msg)"]
        T4["await queue.get()"]
        T5["asyncio.gather(node1, node2, ...)"]
        T6["msg is None（退出哨兵）"]
    end

    subgraph ROS2["ROS2（装好后的写法）"]
        direction TB
        R1["Node 创建的 Topic<br/>类型: sensor_msgs/Imu"]
        R2["DDS 底层消息队列<br/>（自动管理）"]
        R3["publisher.publish(msg)"]
        R4["subscription_callback(msg)"]
        R5["rclpy.spin() / MultiThreadedExecutor"]
        R6["node.destroy_node() / shutdown()"]
    end

    T1 -.-> R1
    T2 -.-> R2
    T3 -.-> R3
    T4 -.-> R4
    T5 -.-> R5
    T6 -.-> R6
```

---

## QoS 策略：消费者跟不上时怎么办

代码里 `maxsize=20` + 满了丢弃，这对应 ROS2 的 **QoS（服务质量）** 配置：

```mermaid
graph TD
    PUB2["Publisher<br/>50 Hz"]

    PUB2 --> Q1["KEEP_LAST (depth=10)<br/>只保留最新 N 条，旧的丢弃"]
    PUB2 --> Q2["KEEP_ALL<br/>全部保留，内存无限增长"]
    PUB2 --> Q3["BEST_EFFORT<br/>尽力发，不保证到达"]

    Q1 --> U1["✅ 传感器数据<br/>（要最新值，丢旧帧没关系）"]
    Q2 --> U2["✅ 控制指令 / 事件<br/>（不能丢）"]
    Q3 --> U3["✅ 视频流<br/>（丢帧可接受，要低延迟）"]

    style Q1 fill:#1a4731,color:#fff,stroke:#2ecc71
    style Q2 fill:#4a2800,color:#fff,stroke:#e67e22
    style Q3 fill:#2d1b4e,color:#fff,stroke:#9b59b6
    style U1 fill:#0d2b1e,color:#ccc,stroke:none
    style U2 fill:#2b1600,color:#ccc,stroke:none
    style U3 fill:#1a0f30,color:#ccc,stroke:none
```

---

## 运行预期输出

```
=== 系统启动 ===

  ⚠  t=123.456s  |accel|=10.01 超阈值!
  ⚠  t=123.476s  |accel|=9.93 超阈值!
  ...
[processor] 共处理 99 帧
[alert_listener] 共收到 N 条告警

=== 所有节点退出 ===
```

---

## 下一步

ROS2 Jazzy 装好后，把 `processor` 改成真正的 ROS2 节点只需要：

```python
# 把这两行
imu_queue = imu_topic.subscribe()
msg = await imu_queue.get()

# 换成这两行
self.sub = self.create_subscription(Imu, '/imu/data', self.callback, 10)
def callback(self, msg: Imu): ...
```

其余逻辑完全不变。
