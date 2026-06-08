# Python（C++ 开发者视角）

> 重点讲和 C++ 思维不同的地方，以及机器人开发中实际用到的部分。

## 和 C++ 的核心差异

| | C++ | Python |
|--|--|--|
| 类型 | 静态，编译期确定 | 动态，运行期确定 |
| 内存 | 手动/RAII | GC 自动回收 |
| 性能 | 高 | 慢（解释执行）|
| 多线程 | 真并行 | GIL 限制（见下）|
| 用途（ROS2）| 高性能节点、驱动 | 算法集成、脚本、原型 |

### GIL（全局解释器锁）

Python 多线程**无法真正并行执行 CPU 密集型任务**，同一时刻只有一个线程在运行 Python 代码。

```python
# CPU 密集型：用多进程，不用多线程
from multiprocessing import Pool

def heavy(x): return x ** 2

with Pool(4) as p:
    results = p.map(heavy, range(1000))

# IO 密集型（网络、文件）：多线程或 asyncio 都可以，GIL 会在 IO 等待时释放
import threading
t = threading.Thread(target=some_io_task)
t.start()
```

---

## 类型注解（Type Hints）

Python 3.5+ 支持，不影响运行时但让代码可读性接近 C++，IDE 能做类型检查。

```python
def move_to(x: float, y: float, speed: float = 1.0) -> bool:
    return True

from typing import Optional, List, Dict, Tuple

def get_path() -> Optional[List[Tuple[float, float]]]:
    return [(0.0, 0.0), (1.0, 2.0)]

# Python 3.10+ 可以直接用 | 代替 Optional
def find(name: str) -> int | None:
    return None
```

---

## dataclass

替代手写 `__init__`，类似 C++ 的结构体。

```python
from dataclasses import dataclass, field

@dataclass
class Pose:
    x: float = 0.0
    y: float = 0.0
    theta: float = 0.0

@dataclass
class Robot:
    name: str
    pose: Pose = field(default_factory=Pose)
    waypoints: list = field(default_factory=list)

r = Robot(name="bot1")
r.pose.x = 1.0
print(r)  # Robot(name='bot1', pose=Pose(x=1.0, y=0.0, theta=0.0), ...)
```

---

## numpy

数值计算核心库，底层是 C，速度接近 C++。

```python
import numpy as np

# 创建数组
a = np.array([1.0, 2.0, 3.0])
m = np.zeros((3, 3))
eye = np.eye(4)            # 4x4 单位矩阵

# 向量化运算（比 Python 循环快 100 倍）
a = np.array([1, 2, 3, 4])
b = a * 2 + 1              # [3, 5, 7, 9]，无需循环

# 矩阵运算（机器人学中常用）
R = np.array([[0, -1], [1, 0]])   # 旋转矩阵
v = np.array([1, 0])
v_rot = R @ v                      # 矩阵乘法，@ 运算符

# 切片
data = np.random.rand(100, 3)      # 100 个三维点
x_coords = data[:, 0]              # 所有点的 x 坐标
subset   = data[data[:, 2] > 0.5]  # 布尔索引：z > 0.5 的点
```

### 点云处理示例

```python
# 激光雷达点云（N×3 数组）
points = np.fromfile("scan.bin", dtype=np.float32).reshape(-1, 4)[:, :3]

# 过滤：只保留前方 5m 内的点
mask   = (points[:, 0] > 0) & (points[:, 0] < 5)
front  = points[mask]

# 计算距离
dist = np.linalg.norm(points, axis=1)   # 每个点到原点的距离

# 统计
print(f"点数: {len(points)}, 最近点: {dist.min():.2f}m")
```

---

## 列表推导 & 生成器

```python
# 列表推导（比 for 循环快，更 Pythonic）
squares = [x**2 for x in range(10)]
evens   = [x for x in range(20) if x % 2 == 0]

# 字典推导
freq = {word: text.count(word) for word in set(text.split())}

# 生成器（惰性求值，不占内存）
def read_sensor_stream():
    while True:
        yield get_latest_reading()   # 按需生成，不一次性加载

for reading in read_sensor_stream():
    process(reading)
```

---

## asyncio（异步 IO）

```python
import asyncio

async def fetch_sensor(url: str) -> dict:
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as resp:
            return await resp.json()

async def main():
    # 并发请求多个传感器（真正并行 IO）
    tasks = [fetch_sensor(url) for url in sensor_urls]
    results = await asyncio.gather(*tasks)

asyncio.run(main())
```

---

## ROS2 Python 节点

```python
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32

class SensorNode(Node):
    def __init__(self):
        super().__init__('sensor_node')

        self.pub = self.create_publisher(Float32, '/sensor/value', 10)
        self.sub = self.create_subscription(
            Float32, '/cmd/speed', self.on_speed, 10
        )
        self.timer = self.create_timer(0.1, self.publish_data)  # 10Hz

    def publish_data(self):
        msg = Float32()
        msg.data = self.read_sensor()
        self.pub.publish(msg)
        self.get_logger().info(f'发布: {msg.data:.2f}')

    def on_speed(self, msg: Float32):
        self.get_logger().info(f'收到速度指令: {msg.data}')

    def read_sensor(self) -> float:
        return 3.14

def main():
    rclpy.init()
    rclpy.spin(SensorNode())
    rclpy.shutdown()
```

---

## 常用标准库速查

```python
# 路径操作
from pathlib import Path
p = Path("/data/scan.bin")
p.exists()          # True/False
p.suffix            # '.bin'
p.stem              # 'scan'
files = list(p.parent.glob("*.bin"))

# JSON 配置文件
import json
with open("config.json") as f:
    cfg = json.load(f)
speed = cfg["robot"]["max_speed"]

# 日志
import logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
logger.info("启动传感器节点")

# 子进程
import subprocess
result = subprocess.run(["ros2", "topic", "list"],
                        capture_output=True, text=True)
print(result.stdout)
```

---

## 面试常问

**Q：Python 的 GIL 是什么，怎么绕过？**

全局解释器锁，保证同一时刻只有一个线程执行 Python 字节码，防止引用计数竞争。绕过方式：CPU 密集用 `multiprocessing`（多进程，各自有 GIL）；调用 C 扩展（numpy、OpenCV 内部释放 GIL）；使用 `concurrent.futures.ProcessPoolExecutor`。

**Q：`*args` 和 `**kwargs` 是什么？**

```python
def f(*args, **kwargs):
    # args：位置参数元组，(1, 2, 3)
    # kwargs：关键字参数字典，{'x': 1, 'y': 2}
    pass

f(1, 2, 3, x=1, y=2)
```

**Q：`__init__` 和 `__new__` 的区别？**

`__new__` 创建对象实例（分配内存，类似 C++ 的 `operator new`），`__init__` 初始化对象（类似构造函数体）。绝大多数情况只需要重写 `__init__`，`__new__` 用于元类、单例等高级场景。
