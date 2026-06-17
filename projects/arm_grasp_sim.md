# 方向 A：机械臂抓取仿真（第 9-12 周）

**技术栈：** C++ · ROS2 Humble · Gazebo Classic · MoveIt2 · Qt QML · rosbridge  
**机器分工：** Windows（仿真 + ROS2） / Mac（Qt QML 界面）  
**简历价值：** 机械臂操控背景 + MoveIt2 + 仿真 + QML 上位机，一次性补齐四个空白

---

## 节点架构

```
Mac (Qt QML)
  │  WebSocket (port 9090)
  ▼
rosbridge_server  [Windows]
  │
  ├─ 订阅 /joint_states        ← state_broadcaster 发布
  ├─ 订阅 /arm/task_status     ← state_broadcaster 发布
  └─ 发布 /arm/grasp_goal      → grasp_task_server 订阅

grasp_task_server  [Action Server]
  └─ 编排抓取流程，调用 arm_motion_client

arm_motion_client  [C++ 封装层]
  └─ 包装 MoveIt2 move_group 接口
```

---

## 第 9 周：机械臂选型 + Gazebo 环境搭建

**选型：Franka Panda**（推荐，`moveit2_tutorials` 官方示例，文档最完整）

### Day 1-2：安装与验证

```bash
# 安装 MoveIt2 和 Panda 相关包
sudo apt install ros-humble-moveit ros-humble-franka-description

# 验证 MoveIt2 + rviz2 跑通
ros2 launch moveit2_tutorials demo.launch.py
```

任务：
- [ ] `demo.launch.py` 能启动，rviz2 中 Panda 可见
- [ ] 用 MotionPlanning 插件手动规划一条路径并执行

### Day 3-4：Gazebo 集成

```bash
sudo apt install ros-humble-gazebo-ros2-control \
                 ros-humble-joint-trajectory-controller \
                 ros-humble-joint-state-broadcaster
```

配置 `joint_trajectory_controller`（ros2_control），验证：

```bash
# 发送关节指令，Gazebo 中机械臂响应
ros2 topic pub /joint_trajectory_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory "{...}"
```

- [ ] Gazebo 启动，Panda 模型加载无报错
- [ ] 发 topic 指令后机械臂能运动

### Day 5-7：场景搭建

- 在 Gazebo 世界文件中添加桌面 + 目标物体（box，尺寸 5×5×5 cm）
- 在 MoveIt2 planning scene 中注册同样的碰撞体
- [ ] **里程碑**：Gazebo 中机械臂 + 桌面 + 目标物体全部可见，planning scene 无碰撞警告

---

## 第 10 周：MoveIt2 运动规划

### Day 1-3：关节空间规划

```cpp
// 核心 API
moveit::planning_interface::MoveGroupInterface move_group(node, "panda_arm");
move_group.setJointValueTarget(target_joints);
move_group.plan(plan);
move_group.execute(plan);
```

- [ ] 能规划 home → 目标关节角并在 Gazebo 中执行
- [ ] 规划失败时有明确错误日志

### Day 4-5：笛卡尔空间规划

```cpp
// 直线路径规划
std::vector<geometry_msgs::msg::Pose> waypoints;
waypoints.push_back(pre_grasp_pose);
waypoints.push_back(grasp_pose);
move_group.computeCartesianPath(waypoints, 0.01, 0.0, trajectory);
```

- [ ] 末端执行器能沿直线运动到目标点
- [ ] 理解 `eef_step`（插值步长）和 `jump_threshold`（跳变检测）参数含义

### Day 6-7：夹爪模拟

选择最简方案：用独立关节组 `panda_hand` 控制夹爪开合，无需额外硬件。

```cpp
move_group_hand.setNamedTarget("open");   // 张开
move_group_hand.setNamedTarget("close");  // 闭合
move_group_hand.move();
```

- [ ] **里程碑**：机械臂能规划路径 → 执行 → 在目标点停止 → 夹爪闭合

---

## 第 11 周：ROS2 节点设计 + 完整抓取流程

### 节点设计

**`arm_motion_client.hpp`**（封装层，非 ROS2 节点）

```cpp
class ArmMotionClient {
public:
    bool moveToPose(const geometry_msgs::msg::Pose& target);
    bool moveToNamed(const std::string& named_target);  // "home", "ready"
    bool moveCartesian(const std::vector<geometry_msgs::msg::Pose>& waypoints);
    bool setGripper(bool open);
};
```

**`grasp_task_server`**（Action Server）

Action 定义：
```
# GraspTask.action
geometry_msgs/Pose target_pose  # 目标物体位姿
---
bool success
string message
---
string phase  # 实时反馈当前阶段
```

抓取序列：
```
PHASE 1: home_position       → moveToNamed("home")
PHASE 2: pre_grasp_pose      → moveToPose(target + offset Z+0.15m)
PHASE 3: grasp_pose          → moveCartesian([grasp_pose])  直线下降
PHASE 4: grasp               → setGripper(false)  夹爪闭合
PHASE 5: lift_pose           → moveCartesian([lift_pose])   直线提升
PHASE 6: home_position       → moveToNamed("home")
```

**`state_broadcaster`**（发布器）

```
发布频率：10 Hz
/arm/joint_states    → sensor_msgs/JointState（透传 /joint_states）
/arm/end_effector    → geometry_msgs/PoseStamped（MoveIt2 计算 FK）
/arm/task_status     → std_msgs/String（IDLE/MOVING/GRASPING/DONE/ERROR）
```

### 验收

```bash
# 手动触发抓取
ros2 action send_goal /arm/grasp_task arm_interfaces/action/GraspTask \
  "{target_pose: {position: {x: 0.4, y: 0.0, z: 0.2}}}"
```

- [ ] **里程碑**：Gazebo 中完整执行六阶段抓取序列，每阶段有 feedback 输出

---

## 第 12 周：Qt QML 界面 + 联调

### Windows 侧：rosbridge 启动

```bash
sudo apt install ros-humble-rosbridge-suite

# 启动 WebSocket 桥接，监听 9090 端口
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

### Mac 侧：Qt QML 界面

**通信封装（C++ 后端）**

```cpp
class RosBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString taskStatus READ taskStatus NOTIFY taskStatusChanged)
    Q_PROPERTY(QVariantList jointAngles READ jointAngles NOTIFY jointAnglesChanged)
    Q_PROPERTY(QVariantMap endEffectorPose READ endEffectorPose NOTIFY poseChanged)
public:
    Q_INVOKABLE void connectToRos(const QString& host, int port = 9090);
    Q_INVOKABLE void sendGraspGoal(double x, double y, double z);
private:
    QWebSocket m_socket;
    void subscribe(const QString& topic, const QString& type);
    void publish(const QString& topic, const QString& type, const QJsonObject& msg);
};
```

**QML 界面布局**

```
┌─────────────────────────────────────┐
│  机械臂抓取监控                       │
│  ● 已连接 192.168.x.x:9090          │
├─────────────────┬───────────────────┤
│  关节角度        │  末端位姿          │
│  J1  ████  12°  │  X   0.412 m      │
│  J2  ██    -8°  │  Y   0.000 m      │
│  J3  █████ 35°  │  Z   0.312 m      │
│  J4  ███  -45°  │  Roll   0.0°      │
│  J5  ██     6°  │  Pitch  0.0°      │
│  J6  ████  90°  │  Yaw    0.0°      │
├─────────────────┴───────────────────┤
│  任务状态：[ MOVING → GRASPING ]     │
│  目标位置  X [0.40] Y [0.00] Z [0.20]│
│            [ 开始抓取 ]              │
├─────────────────────────────────────┤
│  日志                                │
│  [10:23:01] PHASE 2: pre_grasp_pose │
│  [10:23:04] PHASE 3: grasp_pose     │
└─────────────────────────────────────┘
```

### 联调步骤

1. [ ] Mac 用浏览器 WebSocket 客户端（`websocat`）先测试 rosbridge 连通
2. [ ] Qt QML 订阅 `/arm/joint_states`，界面显示实时关节角
3. [ ] Qt QML 订阅 `/arm/task_status`，状态栏联动
4. [ ] Qt QML 发布抓取目标，Gazebo 中机械臂响应
5. [ ] **里程碑**：Mac 界面点击「开始抓取」→ Gazebo 完整执行 → 界面状态实时更新

---

## 简历描述参考

> 基于 ROS2 Humble + MoveIt2 + Gazebo 实现机械臂抓取仿真系统，设计三节点架构（任务编排 Action Server、运动规划封装层、状态广播节点），实现六阶段完整抓取流程；通过 rosbridge WebSocket 与 Qt QML 上位机界面打通，实现关节角度、末端位姿、任务状态实时可视化。
