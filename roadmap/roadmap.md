# 学习路线

目标：3-4 个月内具备投递具身智能 C++ 岗的竞争力。

## 优先级总览

```
P0  ROS2 实战          ← 简历最大空白，面试必问
P0  Python            ← 几乎所有目标 JD 都要求
P1  Qt Quick / QML    ← 现有 Widgets 经验不够，需要补 Quick 栈
P1  Linux IPC 深度     ← ZMQ / DDS / 共享内存，进程通信专项
P2  传感器 + OpenCV    ← 有视觉项目经历是加分项
P2  仿真平台           ← Gazebo / Isaac Sim，加分但非硬门槛
```

---

## 第 1-4 周：ROS2 + Python 并行

### ROS2
- 目标：能独立写多节点程序，理解 Topic / Service / Action / TF
- 路径：[ROS2 Humble 官方教程](https://docs.ros.org/en/humble/Tutorials.html) → 全部完成
- 里程碑：用 ROS2 写一个「发布传感器数据 → 处理节点订阅 → 可视化」的三节点 Demo

### Python
- 目标：能写算法集成脚本、ROS2 Python 节点、数据处理代码
- 重点：numpy / dataclasses / asyncio / 类型注解 / subprocess
- 方式：用 Python 重写现有工作中写过的任意一个工具脚本

---

## 第 5-8 周：Qt QML + Linux IPC

### Qt Quick / QML
- 目标：能用 QML 写一个带实时数据展示的界面，C++ 后端暴露 Q_PROPERTY
- 重点：QML 与 C++ 互操作、Model/View、信号槽跨语言绑定
- 里程碑：把 ROS2 节点数据用 QML 界面实时展示出来（与前面项目打通）

### Linux IPC
- ZMQ：Pub/Sub + Req/Rep，写一个多进程通信框架 Demo
- 补充：共享内存 + 信号量的 POSIX 接口，能解释 DDS 的核心设计思想

---

## 第 9-12 周：综合项目

做一个能放到简历上的项目，覆盖：

```
ROS2 多节点  +  Qt QML 界面  +  传感器数据（可用仿真代替）
```

具体方向二选一：
- **方向 A**：基于 RealSense 深度相机的目标检测与位姿估计（偏视觉）
- **方向 B**：移动机器人导航仿真（Gazebo + Nav2 + Qt 可视化界面）

---

## C++ 面试专项（全程并行）

AI 时代面试官不考「写出来」，考「讲清楚」和「看出来哪里有问题」。需要能张口就说的点：

| 主题 | 重点 |
|------|------|
| 内存管理 | RAII、smart pointer 选型、自定义 deleter |
| 移动语义 | 为什么快、何时触发、完美转发 |
| 并发 | `mutex` vs `atomic`、条件变量、死锁四条件 |
| 虚函数 | vtable 布局、虚析构、纯虚函数 |
| 模板 | SFINAE / `if constexpr`、类型萃取 |
| 性能 | cache miss、内存对齐、避免不必要的拷贝 |

练法：每周选 1 个主题，不看 AI，自己手写一段有代表性的代码 + 能口头解释背后机制。

---

## 时间线

| 阶段 | 时间 | 产出 |
|------|------|------|
| 基础补强 | 第 1-4 周 | ROS2 三节点 Demo + Python 脚本 |
| 技术扩展 | 第 5-8 周 | QML 界面 + ZMQ Demo |
| 综合项目 | 第 9-12 周 | 简历级项目 1 个 |
| 投递 | 第 13 周起 | 简历更新 + 开始海投 |
