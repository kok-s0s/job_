# ROS2 Runtime Demo

对应练习：

- [2026-07-20：ROS2 Workspace 与 C++ Package](/roadmap/daily/2026-07-20)
- [2026-07-21：ROS2 传感器发布节点](/roadmap/daily/2026-07-21)
- [2026-07-22：ROS2 状态订阅节点](/roadmap/daily/2026-07-22)
- [2026-07-23：ROS2 状态查询 Service](/roadmap/daily/2026-07-23)
- [2026-07-24：ROS2 Action 模拟任务](/roadmap/daily/2026-07-24)
- [2026-07-28：C++ RuntimeStateMachine 核心类](/roadmap/daily/2026-07-28)
- [2026-07-29：ROS2 状态机事件接入 Service](/roadmap/daily/2026-07-29)
- [2026-07-30：错误码语义化与故障恢复策略](/roadmap/daily/2026-07-30)
- [2026-07-31：状态机面试讲稿与第 3 周验收](/roadmap/daily/2026-07-31)
- [2026-08-03：统一运行时日志字段设计](/roadmap/daily/2026-08-03)

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
├── srv/
│   └── ApplyRuntimeEvent.srv
└── src/
    ├── action_cancel_test_client.cpp
    ├── runtime_node.cpp
    ├── runtime_state_machine.hpp
    ├── runtime_state_machine_demo.cpp
    ├── runtime_state_machine_review.cpp
    ├── sensor_sim_node.cpp
```

## 这个 demo 在模拟什么

场景：一个机器人运行时系统需要接收底层传感器状态，并根据状态维护自己的运行状态。

- `sensor_sim_node`：模拟底层传感器/驱动层，10Hz 发布 `/robot/imu` 和 `/robot/joint_states`。
- `runtime_node`：模拟机器人运行时/监控层，订阅 `/robot/imu` 和 `/robot/joint_states`，记录接收计数、消息延迟、最近更新时间和 JointState 数据形状。
- `/runtime/reset_fault`：一个 `std_srvs/srv/Trigger` 服务，用来模拟“清故障/复位”命令。
- `/runtime/query_status`：一个 `std_srvs/srv/Trigger` 服务，用来主动查询 runtime 当前状态、接收计数、延迟和 JointState 合法性。
- `/runtime/query_status` 也会返回错误严重级别、是否可恢复、故障原因和恢复建议，方便脚本或 UI 直接展示。
- `/runtime/apply_event`：一个 `robot_runtime_demo/srv/ApplyRuntimeEvent` 服务，用来把外部事件名转换成状态机事件，并返回切换前后状态和故障语义。
- `/runtime/execute_task`：一个自定义 Action，用来执行模拟长耗时任务，持续返回 step/progress feedback，并支持取消。
- `action_cancel_test_client`：自动发送 30 步任务，在收到 feedback 后发起取消，并验证 canceled result。
- `runtime_state_machine.hpp`：纯 C++ 状态机核心，负责 `IDLE/STANDBY/RUNNING/FAULT/RECOVERY` 的转换和错误码维护。
- `runtime_state_machine_demo`：脱离 ROS2 通信的最小状态机验收程序，覆盖正常、故障、恢复和无效事件路径。
- `runtime_state_machine_review`：第 3 周复盘可执行程序，输出状态转换表、错误语义和面试要点。
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
[ok] runtime state machine transitions verified
[ok] week 3 runtime state machine review ready
[sensor_sim_node]: published sensors seq=... imu_ax=... imu_az=9.8 joint_1=...
[runtime_node]: runtime status state=STANDBY runtime_error=NONE runtime_severity=INFO runtime_recoverable=1 runtime_reason=runtime healthy runtime_recovery_hint=no action required imu_count=... joint_count=...
average rate: 9....
std_srvs.srv.Trigger_Response(success=True, message='runtime fault state cleared')
std_srvs.srv.Trigger_Response(success=True, message='state=STANDBY runtime_error=NONE runtime_severity=INFO runtime_recoverable=1 ...')
robot_runtime_demo.srv.ApplyRuntimeEvent_Response(accepted=True, transitioned=True, previous_state='STANDBY', current_state='FAULT', runtime_error='SENSOR_TIMEOUT', message='... recovery_hint=check sensor heartbeat then reset fault')
Goal accepted with ID: ...
Feedback: current_step: ... progress: ...
Result: success: true message: task completed
cancel_result ... success=0 message="task canceled at step ..." feedback_count=...
```

脚本会依次完成：

- 检查 `ros2`、`ROS_DISTRO=humble/jazzy`、`colcon`。
- 执行 `colcon build --packages-select robot_runtime_demo`。
- `source install/setup.bash` 后限时运行 `ros2 launch robot_runtime_demo runtime_demo.launch.py`。
- 运行 `ros2 run robot_runtime_demo runtime_state_machine_demo`，验证纯 C++ 状态机转换表。
- 运行 `ros2 run robot_runtime_demo runtime_state_machine_review`，输出第 3 周状态机复盘表和面试要点。
- 检查 `/robot/imu` 和 `/robot/joint_states` 各能 echo 一条 typed message。
- 检查 `/robot/imu` 和 `/robot/joint_states` 能输出 topic 频率。
- 调用 `/runtime/reset_fault` service。
- 检查 `/runtime/query_status` service 是否存在、类型是否为 `std_srvs/srv/Trigger`，以及响应中是否包含 runtime 状态摘要和错误语义。
- 检查 `/runtime/apply_event` service 是否存在、类型是否为 `robot_runtime_demo/srv/ApplyRuntimeEvent`，并验证 `SensorTimeout -> ResetFault -> RecoveryDone` 故障恢复链。
- 检查故障态 query 输出 `runtime_severity=CRITICAL` 和恢复建议，恢复后 query 输出 `runtime_severity=INFO`。
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
ros2 service type /runtime/apply_event
ros2 service call /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent "{event: SensorTimeout}"
ros2 service call /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent "{event: ResetFault}"
ros2 service call /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent "{event: RecoveryDone}"
ros2 run robot_runtime_demo runtime_state_machine_demo
ros2 run robot_runtime_demo runtime_state_machine_review
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
[ok] runtime state machine transitions verified
[ok] week 3 runtime state machine review ready
[ok] ROS2 runtime demo verified: state machine demo, week 3 review, structured runtime_log fields, typed topics, runtime health, fault metadata, apply_event/query/reset services, and execute_task Action completion/rejection/cancellation are working
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
- 已在 WSL2 Ubuntu 24.04.4 LTS / ROS2 Jazzy 中完成真实 ROS2 验收，最终输出 `[ok] ROS2 runtime demo verified: typed topics, runtime health, query/reset services, and execute_task Action completion/rejection/cancellation are working`。
- 实测合法 goal `{target_steps: 10}` 连续输出 feedback 并以 `SUCCEEDED` 完成；非法 goal `{target_steps: 0}` 被拒绝；取消测试返回 `task canceled at step 4`。

2026-07-27 实现：

- `runtime_node` 新增显式运行时状态机：`IDLE`、`STANDBY`、`RUNNING`、`FAULT`、`RECOVERY`。
- `processRuntimeEvent()` 承接状态转换表，把 `SensorHealthy`、`SensorTimeout`、`JointInvalid`、`StartTask`、`TaskSucceeded`、`TaskCanceled`、`TaskFailed`、`ResetFault`、`RecoveryDone` 落到代码分支。
- 状态摘要新增 `runtime_error`，健康待命时为 `state=STANDBY runtime_error=NONE`，Action 执行期间为 `state=RUNNING`。
- 已在 WSL2 Ubuntu 24.04.4 LTS / ROS2 Jazzy 中完成真实 ROS2 验收，Action 取消时可看到 `runtime transition RUNNING --TaskCanceled--> STANDBY error=NONE`。

2026-07-28 实现：

- 新增 `runtime_state_machine.hpp`，把状态、事件、错误码和 `RuntimeStateMachine::process()` 从 ROS2 node 中抽成纯 C++ 核心。
- 新增 `runtime_state_machine_demo.cpp`，覆盖 9 条有效状态转换，并验证 `Fault + SensorHealthy`、`Standby + TaskSucceeded` 这类无效事件不会修改状态。
- `runtime_node` 改为把 Topic、Service、Action 回调转换为 `RuntimeEvent`，状态转换和错误码维护统一交给 `RuntimeStateMachine`。
- `CMakeLists.txt` 安装 `runtime_state_machine_demo`，`verify_ros2_runtime_demo.sh` 已把它纳入自动验收。
- 已在 WSL2 Ubuntu 24.04.4 LTS / ROS2 Jazzy 中完成真实 ROS2 验收，最终输出包含 `[ok] runtime state machine transitions verified` 和完整 ROS2 demo `[ok]`。

2026-07-29 实现：

- 新增 `srv/ApplyRuntimeEvent.srv`，请求字段为 `event`，响应返回 `accepted`、`transitioned`、前后状态、错误码和消息。
- `runtime_node` 新增 `/runtime/apply_event` service，支持 `SensorHealthy`、`SensorTimeout`、`JointInvalid`、`StartTask`、`TaskSucceeded`、`TaskCanceled`、`TaskFailed`、`ResetFault`、`RecoveryDone`。
- service 回调只做事件名解析和响应组装，状态变化仍统一通过 `RuntimeStateMachine::process()`。
- 验收脚本新增 service list/type 检查、`SensorTimeout -> ResetFault -> RecoveryDone` 恢复链和 `NoSuchEvent` 拒绝检查。

2026-07-30 实现：

- `runtime_state_machine.hpp` 新增 `RuntimeSeverity` 和 `RuntimeErrorInfo`，把错误码映射到严重级别、是否可恢复、故障原因和恢复建议。
- `runtime_node` 的 `/runtime/query_status` 输出新增 `runtime_severity`、`runtime_recoverable`、`runtime_reason`、`runtime_recovery_hint`。
- `/runtime/apply_event` 的响应 message 会携带 severity、recoverable、reason 和 recovery_hint，故障触发时调用方能直接看到下一步操作。
- 验收脚本新增故障态 query 检查，确认 `SensorTimeout` 后为 `runtime_severity=CRITICAL`，恢复后为 `runtime_severity=INFO runtime_recoverable=1`。

2026-07-31 复盘重点：

- 第 3 周产出已经形成一个可运行 ROS2 多节点状态机 demo：Topic 提供传感器流，Service 提供查询/复位/事件驱动，Action 提供可反馈、可取消任务。
- 面试表达重点从“写了状态机”提升为“设计了一个可观测、可恢复、可测试的机器人运行时骨架”。
- 新增 `runtime_state_machine_review`，可通过 `ros2 run robot_runtime_demo runtime_state_machine_review` 输出状态转换表、错误语义和面试要点。
- 最终验收入口是 `bash scripts/verify_ros2_runtime_demo.sh`，覆盖构建、状态机 demo、周复盘程序、Topic、Service、Action、故障语义和恢复链。

2026-08-03 练习重点：

- 第 4 周进入日志、心跳与监控主题，今天先设计统一运行时日志字段。
- 推荐字段包括 `timestamp_ms`、`node`、`level`、`event`、`state`、`runtime_error`、`severity`、`recoverable`、`latency_ms`、`duration_ms` 和 `message`。
- 这些字段后续会服务于 heartbeat、watchdog、耗时统计、自动验收脚本和监控 UI。
- 已在 `runtime_node.cpp` 落地 `[runtime_log]` 统一 key-value 输出，覆盖 heartbeat、状态切换、Service 调用、Action goal/cancel/feedback/result 和 Topic 回调。
- `verify_ros2_runtime_demo.sh` 已检查结构化日志字段，确认 `node=runtime_node`、`event=heartbeat`、`event=state_transition`、`event=service_call` 和 `event=action_feedback` 稳定出现。

注意：Codex 当前是通过提升权限进入这个 WSL 发行版完成验证的；普通 PowerShell 里如果 `wsl -d Ubuntu` 仍提示找不到发行版，需要在你的普通用户上下文中重新初始化/安装 Ubuntu，或把现有发行版导入普通用户。

## 关键点

- `package.xml` 声明 package 元信息，以及 Topic、Service、Action 与接口生成依赖。
- `CMakeLists.txt` 生成 `ExecuteTask` 接口，编译 runtime、传感器节点与 Action 取消测试客户端。
- `srv/ApplyRuntimeEvent.srv` 展示如何定义一个可复盘的状态机事件入口。
- `sensor_sim_node.cpp` 展示 typed Topic publisher，模拟 IMU 和关节状态持续输出数据。
- `runtime_state_machine.hpp` 展示纯 C++ 运行时状态机，便于脱离 ROS2 做快速验证和复盘。
- `runtime_state_machine.hpp` 同时维护错误码语义，统一提供 severity、recoverable、reason 和 recovery_hint。
- `runtime_state_machine_review.cpp` 展示如何把一周成果变成可运行、可检查的复盘输出。
- `runtime_node.cpp` 展示 typed Topic subscriber、状态机事件适配、健康判断、状态查询/故障复位 Service、故障语义输出，以及可反馈、可取消的 Action server。
- 构建后必须 `source install/setup.bash`，否则当前 shell 找不到新 package。
- `scripts/verify_ros2_runtime_demo.sh` 是验收入口，用来补齐环境检查、状态机 demo、构建、launch 启动、Topic 发布/订阅检查。

## ROS2 适合什么应用场景

- 多传感器机器人：相机、雷达、IMU、关节状态分别由不同 node 发布，感知/定位/规划节点订阅这些数据。
- 机器人本体运行时：状态机、心跳、故障检测、复位、任务调度拆成清晰的节点和服务。
- 仿真和实机切换：Gazebo / Isaac Sim 中的模拟数据和真实驱动可以暴露相似 Topic，让上层算法少改代码。
- 分布式系统：ROS2 基于 DDS，天然支持多个进程甚至多台机器之间发现和通信。
- 工程集成：用 launch 管理多节点启动，用 colcon 管理多个 package 的构建，用 rosbag2 记录和回放问题现场。
