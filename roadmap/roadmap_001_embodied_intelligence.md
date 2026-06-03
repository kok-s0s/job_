# 学习路线：具身智能方向（对应 JD-001）

> 适用背景：有 C++/Qt/机械臂软件开发经验，向具身智能赛道跳槽

## 优先级划分

### P0 — 最高优先（2-4 周）

**ROS2 实战**
- 学习资源：[ROS2 官方教程](https://docs.ros.org/en/humble/)
- 重点：Node / Topic / Service / Action / launch 文件
- 练手：写一个多节点的机器人仿真控制程序

**Python 补强**
- 重点：numpy、subprocess、asyncio、类型注解
- 目标：能独立写算法集成脚本和 ROS2 Python 节点

### P1 — 重要（1-2 个月）

**进程间通信**
- ZMQ：学习 Req/Rep、Pub/Sub、Push/Pull 模式，写一个多进程通信 Demo
- DDS：了解 FastDDS 或 CycloneDDS，理解 QoS 概念
- 推荐资料：ZeroMQ Guide（免费在线书）

**视觉算法集成**
- OpenCV：图像处理基础、相机标定、坐标变换
- 相机 SDK：Intel RealSense SDK 或 ZED SDK（选一个熟悉）
- 练手：写一个抓取点识别的 Demo（点云/2D 图像均可）

### P2 — 加分项（持续学习）

**运动规划**
- MoveIt2 基础：碰撞检测、轨迹规划
- 了解 URDF/xacro 机器人模型描述

**物流场景了解**
- 了解拣选（Pick & Place）、码垛的基本流程
- 看京东/快仓/节卡等公司的技术博客

## 项目建议

| 项目 | 技术栈 | 目的 |
|------|--------|------|
| ROS2 多节点机器人控制仿真 | ROS2 + Gazebo | 体现 ROS2 实战能力 |
| ZMQ 多进程通信框架 | C++ + ZMQ | 体现进程间通信能力 |
| 基于 RealSense 的目标检测抓取 | Python + OpenCV + ROS2 | 综合项目，最有说服力 |

## 时间线参考

```
第 1-4 周：  ROS2 基础 + Python 补强
第 5-8 周：  ZMQ/DDS + 视觉基础
第 9-12 周： 综合项目开发
第 13 周+：  简历更新 + 投递
```
