# 项目计划

## 阶段一：练手项目（第 1-8 周）

### P1：ROS2 多节点传感器 Demo
- **技术栈**：ROS2 Humble + C++ + Python
- **内容**：模拟传感器发布节点 → 数据处理节点 → 可视化节点
- **目标**：熟悉 ROS2 核心机制，有可运行的代码
- **状态**：未开始

### P2：ZMQ 多进程通信框架
- **技术栈**：C++ + ZeroMQ
- **内容**：Pub/Sub 和 Req/Rep 两种模式，封装成简单可用的库
- **目标**：掌握 DDS/ZMQ 通信，面试时能讲清楚进程间通信方案选型
- **状态**：未开始

---

## 阶段二：简历级项目（第 9-12 周）

### ✅ 方向 A：机械臂抓取仿真（已选定）

- **技术栈**：C++ + ROS2 Humble + Gazebo Classic + MoveIt2 + Qt QML + rosbridge
- **内容**：Franka Panda 在 Gazebo 中执行完整抓取序列，Qt QML 上位机实时监控
- **亮点**：直接复用机械臂背景，同时补齐 MoveIt2 + 仿真 + QML 三个简历空白
- **详细任务拆分**：[→ 查看第 9-12 周详细计划](arm_grasp_sim.md)

---

## 阶段三：公司 SDK 实战项目（ROKAE SDK × ROS2）

### 方向 B：ROKAE Copilot（公司 SDK 项目）

- **技术栈**：ROKAE SDK / ROS2 + Python/C++ + MoveIt2 + Dashboard
- **内容**：基于公司 SDK 封装机械臂运动、IO、拖动示教、状态读取，做自然语言控制与轨迹回放系统。
- **目标**：形成一个能在公司 SDK 生态内持续迭代的 ROS2 技术项目。
- **详细计划**：[→ 查看 ROKAE SDK × ROS2 项目规划](rokae_ros2_sdk_apps.md)
