# 实战项目

目标：所有项目都服务“机器人软件系统开发工程师”投递，不再放泛前端、兴趣工具或游戏方向内容。

## 主线项目：Humanoid Runtime Mini（第 1-8 周）

这是当前最重要的简历前置项目，用来证明你能做机器人本体软件系统，而不是只会写单个节点。

| 模块 | 技术栈 | 面试价值 |
|------|--------|----------|
| ROS2 多节点骨架 | C++ / ROS2 Humble / colcon | 能讲清 Topic / Service / Action 的边界 |
| 任务状态机 | C++17 / enum class / 回调 | 能讲任务调度、故障恢复、状态管理 |
| 日志与心跳 | rclcpp / watchdog / 耗时统计 | 能讲系统可观察性和异常定位 |
| DDS QoS 实验 | ROS2 QoS profile | 能讲可靠性、延迟、队列深度取舍 |
| 数据录制回放 | rosbag2 / session id | 能讲数据闭环和问题复现 |
| 推理服务 | ONNX Runtime C++ | 能讲 AI 算法工程化落地 |
| CAN 模拟驱动 | SocketCAN / vcan | 能讲执行器通信、超时和故障检测 |

对应计划：[工作日 2 小时量化学习计划](/roadmap/weekday_2h_plan)

## 简历级展示项目（第 9-12 周）

### 方向 A：机械臂抓取仿真

- **技术栈**：C++ / ROS2 Humble / Gazebo Classic / MoveIt2 / Qt QML / rosbridge
- **内容**：Franka Panda 在 Gazebo 中执行完整抓取序列，Qt QML 上位机实时监控
- **亮点**：直接复用机械臂背景，同时补齐 MoveIt2 + 仿真 + QML 三个简历空白
- **详细任务拆分**：[→ 查看第 9-12 周详细计划](arm_grasp_sim.md)

### 方向 B：ROKAE SDK × ROS2

- **技术栈**：ROKAE SDK / ROS2 + Python/C++ + MoveIt2 + Dashboard
- **内容**：基于公司 SDK 封装机械臂运动、IO、拖动示教、状态读取，做自然语言控制与轨迹回放系统。
- **目标**：形成一个能在公司 SDK 生态内持续迭代的 ROS2 技术项目。
- **详细计划**：[→ 查看 ROKAE SDK × ROS2 项目规划](rokae_ros2_sdk_apps.md)
