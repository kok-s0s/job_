# ROS2 Runtime Demo

对应练习：

- [2026-07-20：ROS2 Workspace 与 C++ Package](/roadmap/daily/2026-07-20)
- [2026-07-21：ROS2 传感器发布节点](/roadmap/daily/2026-07-21)
- [2026-07-22：ROS2 状态订阅节点](/roadmap/daily/2026-07-22)
- [2026-07-23：ROS2 状态查询 Service](/roadmap/daily/2026-07-23)
- [2026-07-24：ROS2 Action 模拟任务](/roadmap/daily/2026-07-24)

这是一个最小但完整的 ROS2 C++ package，用来验证 workspace、package、node、typed Topic、Service、Action、launch、`colcon build`、`ros2 run` / `ros2 launch` 的完整流程。

源码目录：

```txt
projects/ros2_runtime_demo/src/robot_runtime_demo
```

## 文件结构

```txt
robot_runtime_demo/
├── action/
│   └── ExecuteTask.action
├── CMakeLists.txt
├── launch/
│   └── runtime_demo.launch.py
├── package.xml
└── src/
    ├── action_cancel_test_client.cpp
    ├── sensor_sim_node.cpp
    └── runtime_node.cpp
```

## 这个 demo 在模拟什么

场景：一个机器人运行时系统需要接收底层传感器状态，并根据状态维护自己的运行状态。

- `sensor_sim_node`：模拟底层传感器/驱动层，10Hz 发布 `/robot/imu` 和 `/robot/joint_states`。
- `runtime_node`：模拟机器人运行时/监控层，订阅 `/robot/imu` 和 `/robot/joint_states`，记录接收计数、消息延迟、最近更新时间和 JointState 数据形状。
- `/runtime/reset_fault`：一个 `std_srvs/srv/Trigger` 服务，用来模拟“清故障/复位”命令。
- `/runtime/query_status`：一个 `std_srvs/srv/Trigger` 服务，用来主动查询 runtime 当前状态、接收计数、延迟和 JointState 合法性。
- `/runtime/execute_task`：一个自定义 Action，用来执行模拟长耗时任务，持续返回 step/progress feedback，并支持取消。
- `action_cancel_test_client`：自动发送 30 步任务，在收到 feedback 后发起取消，并验证 canceled result。
- `runtime_demo.launch.py`：一次启动两个节点，演示 ROS2 多进程节点协作。

这就是 ROS2 最常见的使用方式：把机器人系统拆成多个 node，用 Topic 传连续数据，用 Service 做一次性请求/响应，用 Action 管理可反馈、可取消的长耗时任务，用 launch 管理启动。

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

预期能看到 Topic、Service 和 Action 输出：

```txt
[sensor_sim_node]: published sensors seq=... imu_ax=... imu_az=9.8 joint_1=...
[runtime_node]: runtime status state=RUNNING imu_count=... joint_count=... latest_accel_z=9.80 latest_joint_count=3 imu_latency_ms=... joint_latency_ms=... joint_valid=1
average rate: 9....
std_srvs.srv.Trigger_Response(success=True, message='runtime fault state cleared')
std_srvs.srv.Trigger_Response(success=True, message='state=RUNNING imu_count=... joint_count=... latest_accel_z=9.80 latest_joint_count=3 imu_latency_ms=... joint_latency_ms=... joint_valid=1')
Goal accepted with ID: ...
Feedback: current_step: ... progress: ...
Result: success: true message: task completed
cancel_result ... success=0 message="task canceled at step ..." feedback_count=...
```

脚本会依次完成：

- 检查 `ros2`、`ROS_DISTRO=humble/jazzy`、`colcon`。
- 执行 `colcon build --packages-select robot_runtime_demo`。
- `source install/setup.bash` 后限时运行 `ros2 launch robot_runtime_demo runtime_demo.launch.py`。
- 检查 `/robot/imu` 和 `/robot/joint_states` 各能 echo 一条 typed message。
- 检查 `/robot/imu` 和 `/robot/joint_states` 能输出 topic 频率。
- 调用 `/runtime/reset_fault` service。
- 检查 `/runtime/query_status` service 是否存在、类型是否为 `std_srvs/srv/Trigger`，以及响应中是否包含 runtime 状态摘要。
- 检查 `/runtime/execute_task` Action 的接口、合法 goal 的 feedback/result，以及非法 goal 的拒绝。
- 在 Action 执行期间调用 `/runtime/query_status`，确认 Service 和 heartbeat 没有被耗时任务阻塞。
- 运行 `action_cancel_test_client`，确认取消请求被接受并返回取消时的 step。
- 检查输出中是否出现 typed 传感器发布、运行时订阅日志、topic hz、Service 和 Action 的预期响应。

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
ros2 service type /runtime/query_status
ros2 service call /runtime/query_status std_srvs/srv/Trigger {}
ros2 service call /runtime/reset_fault std_srvs/srv/Trigger {}
ros2 action list -t
ros2 interface show robot_runtime_demo/action/ExecuteTask
ros2 action send_goal --feedback /runtime/execute_task \
  robot_runtime_demo/action/ExecuteTask "{target_steps: 10}"
ros2 run robot_runtime_demo action_cancel_test_client
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
[ok] ROS2 runtime demo verified: typed sensor publishers, runtime health subscriber, query/reset services, topic hz, and launch startup are working
```

本次实测频率：

- `/robot/imu`：`average rate: 9.997` / `10.000`。
- `/robot/joint_states`：`average rate: 10.000`。

2026-07-22 补验：

- `runtime_node` heartbeat 已包含 `imu_latency_ms`、`joint_latency_ms`、`joint_valid=1`。
- `/runtime/reset_fault` service call 返回 `success=True`。

2026-07-23 实现：

- `runtime_node` 新增 `/runtime/query_status` service。
- `runtime_node` 新增 `buildStatusSummary()`，heartbeat 和 query service 复用同一份状态摘要。
- 验证脚本新增 `ros2 service list`、`ros2 service type /runtime/query_status` 和 `ros2 service call /runtime/query_status` 检查。
- 已在 WSL2 Ubuntu 24.04.4 LTS / ROS2 Jazzy 中完成真实 ROS2 验收。
- `ros2 service call /runtime/query_status std_srvs/srv/Trigger "{}"` 返回 `success=True`，message 包含 `state=RUNNING`、接收计数、延迟和 `joint_valid=1`。

2026-07-24 实现：

- 新增自定义 `ExecuteTask.action`：goal 为 `target_steps`，feedback 为 `current_step/progress`，result 为 `success/message`。
- `runtime_node` 新增 `/runtime/execute_task` Action server，拒绝非正数 goal 和并发 goal。
- Action 在独立、可回收的工作线程中执行，每 200 ms 发布一次 feedback，不阻塞 heartbeat、Topic 或 Service callback。
- 正常路径返回 `task completed`；取消路径返回 `task canceled at step N`；ROS2 退出时进入 abort 路径。
- 状态摘要新增 `task_state` 和 `task_step`，方便在任务执行期间通过 `/runtime/query_status` 观察进度。
- 新增 `action_cancel_test_client` 和自动验收步骤，覆盖完成、非法 goal 拒绝、执行期间查询与主动取消。

注意：Codex 当前是通过提升权限进入这个 WSL 发行版完成验证的；普通 PowerShell 里如果 `wsl -d Ubuntu` 仍提示找不到发行版，需要在你的普通用户上下文中重新初始化/安装 Ubuntu，或把现有发行版导入普通用户。

## 关键点

- `package.xml` 声明 package 元信息，以及 Topic、Service、Action 与接口生成依赖。
- `CMakeLists.txt` 生成 `ExecuteTask` 接口，编译 runtime、传感器节点与 Action 取消测试客户端。
- `sensor_sim_node.cpp` 展示 typed Topic publisher，模拟 IMU 和关节状态持续输出数据。
- `runtime_node.cpp` 展示 typed Topic subscriber、健康判断、状态查询/故障复位 Service，以及可反馈、可取消的 Action server。
- 构建后必须 `source install/setup.bash`，否则当前 shell 找不到新 package。
- `scripts/verify_ros2_runtime_demo.sh` 是验收入口，用来补齐环境检查、构建、launch 启动、Topic 发布/订阅检查。

## ROS2 适合什么应用场景

- 多传感器机器人：相机、雷达、IMU、关节状态分别由不同 node 发布，感知/定位/规划节点订阅这些数据。
- 机器人本体运行时：状态机、心跳、故障检测、复位、任务调度拆成清晰的节点和服务。
- 仿真和实机切换：Gazebo / Isaac Sim 中的模拟数据和真实驱动可以暴露相似 Topic，让上层算法少改代码。
- 分布式系统：ROS2 基于 DDS，天然支持多个进程甚至多台机器之间发现和通信。
- 工程集成：用 launch 管理多节点启动，用 colcon 管理多个 package 的构建，用 rosbag2 记录和回放问题现场。
