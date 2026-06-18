# ROKAE SDK × ROS2 项目规划

> 目标：基于公司 ROKAE SDK / ROS2 驱动，做一款能展示机器人二次开发能力、可持续迭代、适合写进简历和内部 demo 的 ROS2 技术项目。

## 资料入口

- SDK 文档总览：<http://sw.rokae.com:8989/docs/SDK>
- SDK 快速开始：<http://sw.rokae.com:8989/docs/SDK/quick_start>
- C++ SDK 场景示例：<http://sw.rokae.com:8989/docs/SDK/cpp/cpp_example>
- ROS2 使用说明：<http://sw.rokae.com:8989/docs/ROS2/rokae_ros2_manual>

## SDK 能力地图

ROKAE SDK / ROS2 文档里能支撑的能力可以分成两层。

### ROS2 应用层

适合快速做应用原型。通过 `rokae_hardware` 启动驱动，再用 ROS2 Service / Topic / Action 操作机器人。

常用接口：

| 类型 | 名称 | 用途 |
| --- | --- | --- |
| Topic | `/rokae_driver/joint_states` | 读取关节角状态 |
| Topic | `/rokae_driver/cartesian_pose` | 读取法兰系笛卡尔位姿 |
| Service | `/rokae_driver/get_robot_info` | 查询型号、SDK 版本等 |
| Service | `/rokae_driver/movej` | 关节空间运动 |
| Service | `/rokae_driver/movel` | 笛卡尔直线运动 |
| Service | `/rokae_driver/movec` | 圆弧运动 |
| Service | `/rokae_driver/jog_control` | Jog 手动控制 |
| Service | `/rokae_driver/drag_control` | 拖动示教开关 |
| Service | `/rokae_driver/calculate_fk` | 正解计算 |
| Service | `/rokae_driver/calculate_ik` | 逆解计算 |
| Service | `/rokae_driver/get_do` / `/rokae_driver/set_do` | 读写数字输出 |
| Service | `/rokae_driver/read_register` / `/rokae_driver/write_register` | 寄存器读写 |
| Action | `/move_action` | MoveIt 规划并执行 |
| Action | `/execute_trajectory` | 执行已规划轨迹 |
| Action | `/position_joint_trajectory_controller/follow_joint_trajectory` | 轨迹跟踪 |

启动驱动示例：

```bash
ros2 launch rokae_hardware rokae_driver.launch.py \
  robot_ip:=192.168.2.160 local_ip:=192.168.2.100
```

连通性检查：

```bash
ros2 service call /rokae_driver/get_robot_info rokae_msgs/srv/GetRobotInfo
ros2 topic echo /rokae_driver/joint_states
ros2 topic echo /rokae_driver/cartesian_pose
```

### SDK 底层控制层

适合做更底层、更高频、更有技术含量的控制。C++ SDK 场景示例覆盖：

- 非实时运动指令：MoveJ / MoveL / MoveC、速度调节、暂停继续、碰撞恢复。
- 实时模式：笛卡尔阻抗、关节伺服、点位跟随、轴空间阻抗、力矩控制。
- 工具能力：IO / 寄存器、末端按键、透传协议、路径录制回放、正逆解、工具/工件/基坐标标定。

项目第一阶段优先用 ROS2 封装应用层能力；等 MVP 跑通后，再下沉到 SDK 实时模式做亮点。

## 推荐项目方向

### 方向 A：语音控制机械臂助手

一句话：用户说自然语言，系统解析成机器人动作。

典型指令：

- “回到拍照位”
- “向左移动 5 厘米”
- “打开夹爪”
- “进入拖动示教”
- “记录当前位置为 A 点”
- “从 A 点直线移动到 B 点”

ROS2 架构：

```mermaid
flowchart LR
  A[语音 / 文本输入] --> B[意图解析节点]
  B --> C[任务编排节点]
  C --> D[ROKAE Motion Client]
  D --> E[/rokae_driver movej/movel/jog/drag]
  D --> F[/rokae_driver set_do]
  G[/joint_states / cartesian_pose] --> C
```

适合做成第一个 MVP。技术闭环清晰，展示效果强，风险可控。

### 方向 B：拖动示教 + 轨迹回放

一句话：打开拖动示教，人工拖一遍，系统记录轨迹并自动回放。

可用接口：

- `drag_control`：进入/退出拖动示教。
- `/joint_states`：记录关节轨迹。
- `/cartesian_pose`：记录末端路径。
- `movej` / `movel` 或 FollowJointTrajectory：回放。

亮点：

- 非常贴近工业现场。
- 能自然扩展出轨迹平滑、速度缩放、安全边界检查。
- 适合做“低代码示教”工具。

### 方向 C：AI 桌面整理机器人

一句话：相机识别桌面物体，机械臂自动分类摆放。

ROS2 节点：

- `perception_node`：目标检测 / 位姿估计。
- `grasp_planner_node`：生成抓取点。
- `motion_planner_node`：调用 MoveIt 或 `movel`。
- `tool_io_node`：控制夹爪 DO。
- `task_manager_node`：状态机编排。

这个方向展示效果最好，但前期依赖视觉、夹爪、标定，建议作为第二阶段。

### 方向 D：机器人状态可视化 Dashboard

一句话：做一个 Web / Qt 上位机，实时显示机器人状态并提供常用操作按钮。

数据源：

- `/rokae_driver/joint_states`
- `/rokae_driver/cartesian_pose`
- controller state
- IO / register service

功能：

- 实时关节角曲线。
- 末端位姿显示。
- IO 状态面板。
- 一键回零、使能/停止、Jog、拖动示教。

这个方向适合补齐 Qt / Web / ROS2 bridge 能力。

### 方向 E：节拍机械臂 / 表演机器人

一句话：根据音乐节拍或手势生成安全范围内的轨迹，让机械臂做表演动作。

适合展厅 demo，但必须严格限制：

- 速度上限。
- 关节软限位。
- 末端工作空间。
- 急停和停止按钮。

建议只在仿真或低速安全区做。

## 首选落地方案：语音控制 + 拖动示教

建议把第一个正式项目定义为：

> **ROKAE Copilot：基于 ROS2 的机械臂自然语言控制与拖动示教系统**

它融合方向 A 和方向 B，既有 AI 交互亮点，又不脱离机器人 SDK 基础能力。

### MVP 功能

1. 连接机器人并读取信息。
2. 实时显示关节状态和末端位姿。
3. 支持文本命令：
   - `home`
   - `move left 5cm`
   - `move up 3cm`
   - `open gripper`
   - `close gripper`
   - `start drag`
   - `stop drag`
4. 支持记录点位：
   - `record A`
   - `record B`
   - `go A`
   - `line A B`
5. 所有运动前做安全检查。

### ROS2 包设计

```text
rokae_copilot_ws/
  src/
    rokae_copilot_interfaces/
      srv/
        ExecuteCommand.srv
        RecordPose.srv
      msg/
        RobotSnapshot.msg
    rokae_copilot_core/
      rokae_motion_client.py
      task_manager.py
      safety_guard.py
      pose_store.py
    rokae_copilot_ui/
      web_dashboard.py 或 qt_dashboard.cpp
    rokae_copilot_demo/
      launch/
        demo.launch.py
```

### 节点职责

| 节点 | 职责 |
| --- | --- |
| `rokae_motion_client` | 封装 ROKAE ROS2 service/action 调用 |
| `task_manager` | 把命令转为动作序列 |
| `safety_guard` | 工作空间、速度、点位合法性检查 |
| `pose_store` | 保存和读取命名点位 |
| `dashboard` | 展示状态、发送命令 |

### 第一版技术路线

1. **第 1 天：连通性**
   - 启动 `rokae_driver`。
   - 调通 `get_robot_info`。
   - 订阅 `/joint_states`、`/cartesian_pose`。

2. **第 2-3 天：动作封装**
   - 写 `RokaeMotionClient`。
   - 封装 `movej`、`movel`、`jog_control`、`drag_control`、`set_do`。

3. **第 4-5 天：命令系统**
   - 支持文本命令解析。
   - 接入安全检查。
   - 支持点位记录和回放。

4. **第 6-7 天：可视化**
   - 做简单 dashboard。
   - 显示关节角、末端位姿、当前任务状态。

5. **第二周：增强**
   - 接入语音识别。
   - 增加 MoveIt 规划。
   - 增加轨迹录制/回放。

## 安全边界

任何真机项目都先默认低速、安全区、可随时停止。

必须做：

- 首次只用仿真或空载低速。
- 所有相对运动限制最大位移，例如单步不超过 5 cm。
- 所有命令经过白名单解析，不直接执行自由文本。
- 点位记录时保存时间、关节角、笛卡尔位姿、命名来源。
- UI 上保留停止按钮。
- 真机调试时机器人周围不站人。

## 简历表达

可以写成：

> 基于 ROKAE SDK 与 ROS2 设计机械臂智能控制系统，封装机器人运动、IO、拖动示教和状态读取接口；构建任务编排节点与安全检查模块，实现自然语言控制、点位记录、轨迹回放和实时状态可视化。

技术关键词：

- ROS2 Service / Topic / Action
- MoveIt2
- ros2_control
- ROKAE SDK
- C++ / Python
- 机器人运动规划
- 拖动示教
- 安全状态机
- Qt / Web Dashboard

