# 学习路线

目标：3-4 个月内具备投递具身智能 C++ 岗的竞争力。

## 机器分工

| 机器 | 用途 |
|------|------|
| Mac（8GB）| C++ 深度、Python、Qt QML、面试专项 |
| Windows | ROS2、仿真、传感器项目（WSL2 + Ubuntu 22.04）|

---

## 优先级总览

```
P0  ROS2 实战        [Windows]  ← 简历最大空白
P0  Python          [Mac]      ← 几乎所有目标 JD 都要求
P1  Qt Quick / QML  [Mac]      ← 现有 Widgets 不够，补 Quick 栈
P1  Linux IPC 深度  [Windows]  ← ZMQ / DDS / 共享内存
P2  传感器 + OpenCV [Windows]  ← 视觉项目经历是加分项
P2  仿真平台        [Windows]  ← Gazebo / Isaac Sim
```

---

## Mac 任务线

### Python（第 1-3 周）
- 目标：能写算法集成脚本、数据处理代码、自动化工具
- 重点：numpy / dataclasses / asyncio / 类型注解
- 练法：把工作中写过的任意一个 C++ 工具用 Python 重写一遍

### Qt Quick / QML（第 4-8 周）
- 目标：能用 QML 写带实时数据展示的界面，C++ 后端暴露 `Q_PROPERTY`
- 重点：QML 与 C++ 互操作、Model/View、信号槽跨语言绑定、Qt Quick Controls
- 里程碑：写一个模拟传感器数据推送到 QML 界面实时刷新的 Demo

### C++ 面试专项（全程，每周 1 个主题）
不看 AI，自己手写代码 + 能口头解释背后机制。

| 周次 | 主题 | 重点 |
|------|------|------|
| 第 1 周 | 内存管理 | RAII、smart pointer 选型、自定义 deleter |
| 第 2 周 | 移动语义 | 为什么快、何时触发、完美转发 `std::forward` |
| 第 3 周 | 并发 | `mutex` vs `atomic`、条件变量、死锁四条件 |
| 第 4 周 | 虚函数 | vtable 布局、虚析构、多态切片问题 |
| 第 5 周 | 模板 | `if constexpr`、SFINAE、类型萃取 |
| 第 6 周 | 性能 | cache miss、内存对齐、避免不必要的拷贝 |
| 第 7-8 周 | 综合复习 | 用上面任意主题出题互练 |

---

## Windows 任务线

> 环境：WSL2 + Ubuntu 22.04 + ROS2 Humble

### 环境搭建（第 1 周）
1. 安装 WSL2，选 Ubuntu 22.04
2. 安装 ROS2 Humble（Desktop 完整版）
3. 跑通 `talker` / `listener` 官方示例

### ROS2 实战（第 2-5 周）
- 路径：[ROS2 Humble 官方教程](https://docs.ros.org/en/humble/Tutorials.html) 全部完成
- 重点：Node / Topic / Service / Action / TF2 / launch 文件 / colcon 构建
- 里程碑：写一个「发布传感器数据 → 处理节点订阅 → 可视化（rviz2）」三节点 Demo

### Linux IPC + ZMQ（第 6-8 周）
- ZMQ：Pub/Sub + Req/Rep，封装一个简单的多进程通信 Demo
- 补充：共享内存 + 信号量 POSIX 接口，能解释 DDS 的设计思想

---

## 综合项目（第 9-12 周，Windows 主导 + Mac QML 界面）

两台机器合流，做一个完整的简历级项目。二选一：

### 方向 A：移动机器人导航仿真（推荐，不需要硬件）
```
Gazebo 仿真 + Nav2 导航栈 + ROS2 + Qt QML 监控界面
```
- Windows：Gazebo 仿真 + Nav2 + ROS2 节点
- Mac：Qt QML 界面，通过 rosbridge 或自定义 WebSocket 接收数据

### 方向 B：目标检测与位姿估计（需要 RealSense 相机）
```
RealSense 深度相机 + OpenCV/YOLO + ROS2 + Qt QML 可视化
```

---

## 时间线

| 阶段 | 时间 | Mac | Windows |
|------|------|-----|---------|
| 基础补强 | 第 1-4 周 | Python + C++面试专项 | WSL2 环境 + ROS2 入门 |
| 技术扩展 | 第 5-8 周 | Qt QML Demo | ROS2 进阶 + ZMQ |
| 综合项目 | 第 9-12 周 | QML 界面 | 仿真 + ROS2 节点 |
| 投递 | 第 13 周起 | 简历更新 + 开始投递 | — |
