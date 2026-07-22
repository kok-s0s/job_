# ROS2 Runtime Demo

对应练习：

- [2026-07-20：ROS2 Workspace 与 C++ Package](/roadmap/daily/2026-07-20)
- [2026-07-21：ROS2 传感器发布节点](/roadmap/daily/2026-07-21)
- [2026-07-22：ROS2 状态订阅节点](/roadmap/daily/2026-07-22)

这是一个最小但完整的 ROS2 C++ package，用来验证 workspace、package、node、typed Topic、Service、launch、`colcon build`、`ros2 run` / `ros2 launch` 的完整流程。

源码目录：

```txt
projects/ros2_runtime_demo/src/robot_runtime_demo
```

## 文件结构

```txt
robot_runtime_demo/
├── CMakeLists.txt
├── launch/
│   └── runtime_demo.launch.py
├── package.xml
└── src/
    ├── sensor_sim_node.cpp
    └── runtime_node.cpp
```

## 这个 demo 在模拟什么

场景：一个机器人运行时系统需要接收底层传感器状态，并根据状态维护自己的运行状态。

- `sensor_sim_node`：模拟底层传感器/驱动层，10Hz 发布 `/robot/imu` 和 `/robot/joint_states`。
- `runtime_node`：模拟机器人运行时/监控层，订阅 `/robot/imu` 和 `/robot/joint_states`，记录接收计数、消息延迟、最近更新时间和 JointState 数据形状。
- `runtime/reset_fault`：一个 `std_srvs/srv/Trigger` 服务，用来模拟“清故障/复位”命令。
- `runtime_demo.launch.py`：一次启动两个节点，演示 ROS2 多进程节点协作。

这就是 ROS2 最常见的使用方式：把机器人系统拆成多个 node，用 Topic 传连续数据，用 Service 做一次性请求/响应，用 launch 管理启动。

## 环境搭建

推荐环境：

- Windows 11 + WSL2
- Ubuntu 24.04 + ROS2 Jazzy，或 Ubuntu 22.04 + ROS2 Humble

在 Ubuntu 中执行：

```bash
cd projects/ros2_runtime_demo
bash scripts/setup_ros2_ubuntu.sh
```

安装完成后确认：

```bash
source /opt/ros/jazzy/setup.bash   # Ubuntu 24.04
# source /opt/ros/humble/setup.bash  # Ubuntu 22.04
which ros2
echo $ROS_DISTRO
which colcon
```

## 在 WSL2 / Ubuntu / ROS2 中验证

把本目录作为 workspace 使用：

```bash
cd projects/ros2_runtime_demo
bash scripts/verify_ros2_runtime_demo.sh
```

预期能看到四类输出：

```txt
[sensor_sim_node]: published sensors seq=... imu_ax=... imu_az=9.8 joint_1=...
[runtime_node]: runtime status=RUNNING imu_received=... joint_received=... latest_accel_z=9.80 latest_joints=3 imu_latency_ms=... joint_latency_ms=... joint_valid=1
average rate: 9....
std_srvs.srv.Trigger_Response(success=True, message='runtime fault state cleared')
```

脚本会依次完成：

- 检查 `ros2`、`ROS_DISTRO=humble/jazzy`、`colcon`。
- 执行 `colcon build --packages-select robot_runtime_demo`。
- `source install/setup.bash` 后限时运行 `ros2 launch robot_runtime_demo runtime_demo.launch.py`。
- 检查 `/robot/imu` 和 `/robot/joint_states` 各能 echo 一条 typed message。
- 检查 `/robot/imu` 和 `/robot/joint_states` 能输出 topic 频率。
- 调用 `/runtime/reset_fault` service。
- 检查输出中是否出现 typed 传感器发布、运行时订阅日志、topic hz 和 service 成功响应。

如果想手动分步验证，也可以执行：

```bash
cd projects/ros2_runtime_demo
source /opt/ros/jazzy/setup.bash   # Ubuntu 24.04
# source /opt/ros/humble/setup.bash  # Ubuntu 22.04
colcon build --packages-select robot_runtime_demo
source install/setup.bash
ros2 launch robot_runtime_demo runtime_demo.launch.py
```

另开一个 Ubuntu 终端，可以观察 Topic 和调用 Service：

```bash
source /opt/ros/jazzy/setup.bash   # Ubuntu 24.04
# source /opt/ros/humble/setup.bash  # Ubuntu 22.04
cd projects/ros2_runtime_demo
source install/setup.bash

ros2 topic list
ros2 topic echo /robot/imu --once
ros2 topic echo /robot/joint_states --once
ros2 topic hz /robot/imu
ros2 topic hz /robot/joint_states
ros2 service list
ros2 service call /runtime/reset_fault std_srvs/srv/Trigger {}
```

## 已验证环境

当前机器已在 WSL2 `Ubuntu` 中跑通：

```txt
Ubuntu 24.04.4 LTS
ROS_DISTRO=jazzy
ros2=/opt/ros/jazzy/bin/ros2
colcon=/usr/bin/colcon
```

验证脚本最终输出：

```txt
[ok] ROS2 runtime demo verified: typed sensor publishers, runtime health subscriber, topic hz, service call, and launch startup are working
```

本次实测频率：

- `/robot/imu`：`average rate: 9.997` / `10.000`。
- `/robot/joint_states`：`average rate: 10.000`。

2026-07-22 补验：

- `runtime_node` heartbeat 已包含 `imu_latency_ms`、`joint_latency_ms`、`joint_valid=1`。
- `/runtime/reset_fault` service call 返回 `success=True`。

注意：Codex 当前是通过提升权限进入这个 WSL 发行版完成验证的；普通 PowerShell 里如果 `wsl -d Ubuntu` 仍提示找不到发行版，需要在你的普通用户上下文中重新初始化/安装 Ubuntu，或把现有发行版导入普通用户。

## 关键点

- `package.xml` 声明 package 元信息和 `rclcpp`、`sensor_msgs`、`std_srvs` 依赖。
- `CMakeLists.txt` 查找 `ament_cmake`、`rclcpp`、`sensor_msgs`、`std_srvs`，编译并安装两个节点和 launch 文件。
- `sensor_sim_node.cpp` 展示 typed Topic publisher，模拟 IMU 和关节状态持续输出数据。
- `runtime_node.cpp` 展示 typed Topic subscriber、延迟统计、JointState 形状检查、topic 超时健康判断和 Service server，模拟机器人运行时根据传感器数据更新状态，并响应外部复位请求。
- 构建后必须 `source install/setup.bash`，否则当前 shell 找不到新 package。
- `scripts/verify_ros2_runtime_demo.sh` 是验收入口，用来补齐环境检查、构建、launch 启动、Topic 发布/订阅检查。

## ROS2 适合什么应用场景

- 多传感器机器人：相机、雷达、IMU、关节状态分别由不同 node 发布，感知/定位/规划节点订阅这些数据。
- 机器人本体运行时：状态机、心跳、故障检测、复位、任务调度拆成清晰的节点和服务。
- 仿真和实机切换：Gazebo / Isaac Sim 中的模拟数据和真实驱动可以暴露相似 Topic，让上层算法少改代码。
- 分布式系统：ROS2 基于 DDS，天然支持多个进程甚至多台机器之间发现和通信。
- 工程集成：用 launch 管理多节点启动，用 colcon 管理多个 package 的构建，用 rosbag2 记录和回放问题现场。
