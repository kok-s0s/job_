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
- [2026-08-04：ROS2 心跳发布与订阅设计](/roadmap/daily/2026-08-04)
- [2026-08-05：Watchdog 监控节点与超时 Fault](/roadmap/daily/2026-08-05)
- [2026-08-06：记录关键耗时与性能观测](/roadmap/daily/2026-08-06)
- [2026-08-07：第 1 阶段集成验收](/roadmap/daily/2026-08-07)
- [2026-08-10：DDS QoS 基础对比](/roadmap/daily/2026-08-10)
- [2026-08-11：传感器数据 best effort 验收实验](/roadmap/daily/2026-08-11)
- [2026-08-12：状态命令 reliable 验收设计](/roadmap/daily/2026-08-12)
- [2026-08-13：queue depth 对延迟的影响实验](/roadmap/daily/2026-08-13)
- [2026-08-14：QoS 面试表达与第 5 周验收](/roadmap/daily/2026-08-14)
- [2026-08-18：rosbag2 录制运行时 Topic](/roadmap/daily/2026-08-18)
- [2026-08-19：给运行时日志增加 session id](/roadmap/daily/2026-08-19)
- [2026-08-20：固定触发一次故障复现](/roadmap/daily/2026-08-20)
- [2026-08-21：数据闭环笔记与第 6 周验收](/roadmap/daily/2026-08-21)
- [2026-08-27：接入 ROS2 推理节点](/roadmap/daily/2026-08-27)

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
    ├── heartbeat_monitor_node.cpp
    ├── runtime_node.cpp
    ├── runtime_integration_review.cpp
    ├── runtime_state_machine.hpp
    ├── runtime_state_machine_demo.cpp
    ├── runtime_state_machine_review.cpp
    ├── sensor_sim_node.cpp
    ├── watchdog_node.cpp
```

## 这个 demo 在模拟什么

场景：一个机器人运行时系统需要接收底层传感器状态，并根据状态维护自己的运行状态。

- `sensor_sim_node`：模拟底层传感器/驱动层，10Hz 发布 `/robot/imu` 和 `/robot/joint_states`。
- `sensor_sim_node` 同时每秒向 `/runtime/heartbeat` 发布自身心跳。
- `runtime_node`：模拟机器人运行时/监控层，订阅 `/robot/imu` 和 `/robot/joint_states`，记录接收计数、消息延迟、最近更新时间和 JointState 数据形状。
- `runtime_node` 同时每秒向 `/runtime/heartbeat` 发布带状态机字段的心跳。
- `heartbeat_monitor_node`：订阅 `/runtime/heartbeat`，维护每个节点的 `last_seq`、`last_stamp_ms`、`age_ms` 和 `status`。
- `watchdog_node`：订阅 `/runtime/heartbeat`，维护每个节点最后一次心跳时间，每 1 秒检查一次。若某节点 3 秒无心跳则判定 TIMEOUT，并通过 `/runtime/apply_event` Service 发送 `SensorTimeout` 事件，让 `runtime_node` 进入 Fault。带启动宽限期，避免冷启动误报。
- `/runtime/reset_fault`：一个 `std_srvs/srv/Trigger` 服务，用来模拟“清故障/复位”命令。
- `/runtime/query_status`：一个 `std_srvs/srv/Trigger` 服务，用来主动查询 runtime 当前状态、接收计数、延迟和 JointState 合法性。
- `/runtime/query_status` 也会返回错误严重级别、是否可恢复、故障原因和恢复建议，方便脚本或 UI 直接展示。
- `/runtime/apply_event`：一个 `robot_runtime_demo/srv/ApplyRuntimeEvent` 服务，用来把外部事件名转换成状态机事件，并返回切换前后状态和故障语义。
- `/runtime/execute_task`：一个自定义 Action，用来执行模拟长耗时任务，持续返回 step/progress feedback，并支持取消。
- `action_cancel_test_client`：自动发送 30 步任务，在收到 feedback 后发起取消，并验证 canceled result。
- `runtime_state_machine.hpp`：纯 C++ 状态机核心，负责 `IDLE/STANDBY/RUNNING/FAULT/RECOVERY` 的转换和错误码维护。
- `runtime_state_machine_demo`：脱离 ROS2 通信的最小状态机验收程序，覆盖正常、故障、恢复和无效事件路径。
- `runtime_state_machine_review`：第 3 周复盘可执行程序，输出状态转换表、错误语义和面试要点。
- `runtime_integration_review`：第 1 阶段集成验收复盘程序，输出系统拓扑、证据清单、项目表达和 QoS 入口问题。
- `runtime_demo.launch.py`：一次启动 `sensor_sim_node`、`runtime_node`、`heartbeat_monitor_node` 和 `watchdog_node` 四个节点，演示 ROS2 多进程节点协作。

## watchdog 验证结果（WSL2 / ROS2 Jazzy）

**正常心跳**：3 个节点均判定 `status=OK`，`age_ms` 稳定在 ~990ms，`last_seq` 递增正常。

```txt
[watchdog] node=heartbeat_monitor_node status=OK age_ms=995 last_seq=1
[watchdog] node=runtime_node status=OK age_ms=962 last_seq=1
[watchdog] node=sensor_sim_node status=OK age_ms=990 last_seq=1
```

**超时 Fault**：杀掉 `sensor_sim_node` 后，其 `age_ms` 递增超过 3000ms 阈值判定 `TIMEOUT`，并成功触发 Fault：

```txt
[WARN] [watchdog] node=sensor_sim_node status=TIMEOUT age_ms=3990 last_seq=3
[WARN] [watchdog] fault triggered accepted=1 transitioned=0 current_state=FAULT error=SENSOR_TIMEOUT
```

`transitioned=0` 是因为 `runtime_node` 自己也检测到传感器超时已先进入 FAULT，watchdog 触发是冗余安全兜底。可复现脚本：`scripts/test_watchdog_timeout.sh`。

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
[heartbeat_table] node=sensor_sim_node last_seq=... last_stamp_ms=... age_ms=... status=OK
[heartbeat_table] node=runtime_node last_seq=... last_stamp_ms=... age_ms=... status=OK
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
- 检查 `/robot/imu`、`/robot/joint_states` 和 `/runtime/heartbeat` 各能 echo 一条 typed message。
- 检查 `sensor_sim_node`、`runtime_node`、`heartbeat_monitor_node` 的心跳发布、订阅和监控表输出。
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
[ok] phase 1 ROS2 runtime integration review ready
[ok] week 3 runtime state machine review ready
[ok] ROS2 runtime demo verified: state machine demo, phase 1 integration review, week 3 review, structured runtime_log fields, heartbeat pub/sub, watchdog timeout check, performance metrics, typed topics, runtime health, fault metadata, apply_event/query/reset services, and execute_task Action completion/rejection/cancellation are working
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

2026-08-04 练习重点：

- 第 4 周周二开始设计 ROS2 心跳发布与订阅，为 watchdog 监控节点做铺垫。
- 心跳建议通过 `/runtime/heartbeat` Topic 发布，先用 `std_msgs/msg/String` 承载 key-value 字段，后续可升级为自定义 msg。
- 推荐心跳字段包括 `node`、`seq`、`stamp_ms`、`state`、`runtime_error`、`severity`、`recoverable`、`status` 和 `message`。
- 已在 ROS2 demo 中落地 `/runtime/heartbeat` Topic：`sensor_sim_node`、`runtime_node`、`heartbeat_monitor_node` 每秒发布心跳，`heartbeat_monitor_node` 订阅并维护每个节点的 `last_stamp_ms` 和 `age_ms`。
- `verify_ros2_runtime_demo.sh` 已检查 3 个节点的 `[heartbeat]`、`[heartbeat_rx]`、`[heartbeat_table]` 输出。
- 明天 watchdog 的核心规则是：超过 3 秒未收到指定节点心跳，则标记 `TIMEOUT`，再触发 Fault 或报警。

2026-08-06 练习重点：

- 第 4 周周四整理关键耗时指标，为周五第 1 阶段集成验收做准备。
- 当前 demo 已能观察 `latency_ms`、`duration_ms`、`imu_latency_ms`、`joint_latency_ms`、`age_ms` 和 `task_step`。
- 排障时先区分链路延迟和本地处理耗时：`latency_ms` 是“路上花多久”，`duration_ms` 是“回调自己处理多久”。
- `heartbeat_monitor_node` 和 `watchdog_node` 输出的 `age_ms` 用来判断心跳新鲜度，超过阈值时 watchdog 会进入 TIMEOUT 判断。
- 已在 `runtime_node.cpp` 落地 `[perf]` 性能聚合输出，包含 `imu_latency_ms`、`joint_latency_ms`、`max_callback_duration_ms`、`heartbeat_duration_ms`、`last_action_duration_ms` 和 `task_step`。
- `/runtime/query_status` 响应已包含 `max_callback_duration_ms` 和 `last_action_duration_ms`，验收脚本已检查这些字段。
- 今日任务把这些指标整理成可复盘、可面试表达的性能观测清单，明天再做第 1 阶段集成验收。

2026-08-07 练习重点：

- 第 4 周周五做第 1 阶段集成验收，不新增大功能，重点确认 ROS2 Runtime Demo 已形成完整闭环。
- 验收入口仍是 `bash scripts/verify_ros2_runtime_demo.sh`，覆盖构建、launch、多节点启动、Topic、Service、Action、状态机、故障恢复、心跳、watchdog 和性能指标。
- 已新增 `runtime_integration_review`，用可执行程序输出第 1 阶段系统拓扑、6 类证据清单、项目表达和第 5 周 QoS 入口问题。
- `verify_ros2_runtime_demo.sh` 已检查 `runtime_integration_review` 输出，最终 `[ok]` 行包含 `phase 1 integration review`。
- 正常 Action 验收任务调整为 30 步，确保脚本中的 `/runtime/query_status` 能稳定验证执行期 `state=RUNNING task_state=RUNNING`。
- 今日复盘要把零散能力组织成项目表达：Topic 负责连续传感器数据，Service 负责查询/复位/事件注入，Action 负责可反馈、可取消的长任务。
- 关键证据包括 `[runtime_log]`、`[heartbeat_table]`、`[watchdog]`、`[perf]`、`/runtime/query_status` 响应和 Action result。
- 今天的输出是第 1 阶段项目讲解稿，并为第 5 周 DDS QoS 主题留下入口问题。

2026-08-10 练习重点：

- 第 5 周进入 DDS QoS 与通信可靠性，今天先理解 `reliability`、`durability`、`history` 和 `depth`。
- QoS 选择要按数据语义区分：高频传感器流优先实时性，状态命令和故障事件优先确定性，心跳要平衡可靠性和误报风险。
- 已新增 `runtime_qos.hpp`，集中定义传感器流和心跳 QoS，避免多个节点各写一份裸 `10`。
- `/robot/imu` 和 `/robot/joint_states` 更关注最新值，已配置为 `best_effort + volatile + keep_last(5)`，避免旧数据堆积。
- `/runtime/heartbeat` 是 watchdog 的依据，低频但语义重要，已配置为 `reliable + volatile + keep_last(3)`。
- `/runtime/apply_event` 和 `/runtime/execute_task` 表达控制语义，需要明确成功、失败、取消或拒绝，不应无声丢失。
- 节点启动时输出 `[qos]` 证据，验收脚本同时用 `ros2 topic echo --qos-reliability best_effort` 读取传感器样本，用 `ros2 topic info --verbose` 检查 DDS 可见的 `Reliability: BEST_EFFORT` 和 `Durability: VOLATILE`。
- ROS2 完整验收脚本已通过，最终 `[ok]` 行包含 `DDS QoS profiles`。

2026-08-11 练习重点：

- 第 5 周周二专门验收传感器数据的 best effort 行为，把昨天的 QoS 配置变成可观察实验。
- 复盘入口是 `runtime_qos.hpp`、`sensor_sim_node.cpp` 和 `runtime_node.cpp`，确认 `/robot/imu` 与 `/robot/joint_states` 的发布/订阅都使用 `sensorStreamQos()`。
- 手动验收建议使用 `ros2 topic echo /robot/imu --once --qos-reliability best_effort` 和 `ros2 topic echo /robot/joint_states --once --qos-reliability best_effort`。
- 对照实验是尝试 reliable subscriber 订阅 best effort publisher，理解 QoS 不兼容时可能订阅不到消息。
- `ros2 topic info --verbose` 用来复查 DDS 可见字段，重点看 `Reliability: BEST_EFFORT` 和 `Durability: VOLATILE`；depth 仍以代码和 `[qos]` 日志为准。
- 已新增 `runtime_sensor_qos_review`，输出传感器 QoS 表、可靠性不匹配说明和 1 分钟面试表达。
- `verify_ros2_runtime_demo.sh` 已纳入 best effort echo、reliable mismatch、`topic info --verbose` 和 `runtime_sensor_qos_review` 检查。
- ROS2 完整验收脚本已通过，最终 `[ok]` 行包含 `sensor best-effort QoS review`；reliable mismatch 对照输出 `Last incompatible policy: RELIABILITY`。
- 今日复盘目标是把“传感器流优先实时性，控制命令优先确定性”讲成一个工程判断，而不是单纯参数背诵。

2026-08-12 练习重点：

- 第 5 周周三转向状态命令和控制链路，理解为什么 Service / Action / 状态机事件必须有可靠确认。
- 复盘入口是 `runtime_node.cpp`、`srv/ApplyRuntimeEvent.srv`、`action/ExecuteTask.action` 和 `runtime_state_machine.hpp`。
- `/runtime/query_status` 与 `/runtime/reset_fault` 用 Service 表达一次性查询和复位，必须返回明确 response。
- `/runtime/apply_event` 返回 `accepted`、`transitioned`、`previous_state`、`current_state` 和 `runtime_error`，用来区分“事件合法”和“状态真的转换”。
- `/runtime/execute_task` 用 Action 表达长耗时任务，验收 goal accepted、feedback、result、rejection 和 cancel。
- 已新增 `runtime_command_reliability_review`，把 query/reset/apply_event/execute_task 的可靠确认语义整理成可运行复盘输出。
- `verify_ros2_runtime_demo.sh` 已纳入 command reliability review 检查，最终 `[ok]` 行包含 `command reliability review`。
- 今日复盘目标是把“可靠通信不是所有数据都 reliable，而是关键控制命令必须有可验证结果”讲清楚。

2026-08-13 练习重点：

- 第 5 周周四聚焦 `history + depth`，理解 Topic 队列长度如何影响延迟和旧数据积压。
- 复盘入口是 `runtime_qos.hpp`、`sensor_sim_node.cpp`、`runtime_node.cpp`、`heartbeat_monitor_node.cpp` 和 `watchdog_node.cpp`。
- `/robot/imu` 与 `/robot/joint_states` 的核心判断是保持最新状态，depth 太大可能让慢订阅者处理过期传感器样本。
- `/runtime/heartbeat` 需要结合 `age_ms` 判断新鲜度，不能只靠堆积旧心跳来证明节点还活着。
- 今天先设计 depth=1 / 5 / 20 的对比实验，把 `imu_latency_ms`、`joint_latency_ms` 和 backlog 现象联系起来。
- 已新增 `runtime_queue_depth_review`，输出当前 depth 配置、depth=1 / 5 / 20 对比矩阵、backlog 条件和面试总结。
- `verify_ros2_runtime_demo.sh` 已纳入 queue depth review 检查，最终 `[ok]` 行包含 `queue depth review`。
- 本次 ROS2 实测 `/runtime/query_status` 中可观察到 `imu_latency_ms`、`joint_latency_ms` 和 `max_callback_duration_ms`，用于后续慢订阅者实验对比。
- 今日复盘目标是把“queue depth 是延迟和丢弃之间的工程取舍”讲清楚。

2026-08-14 练习重点：

- 第 5 周周五做 QoS 面试表达和周验收，把 reliability、durability、history、depth 组织成一套项目解释。
- 已新增 `runtime_qos_interview_review`，输出本周 QoS 决策表、tradeoff notes 和 interview summary。
- `verify_ros2_runtime_demo.sh` 已纳入 QoS interview review 检查，最终 `[ok]` 行包含 `QoS interview review`。
- 今日复盘目标是把“QoS 选择先看数据语义，再看可靠性、durability、depth 和可观测证据”讲清楚。

2026-08-18 练习重点：

- 第 6 周进入数据采集、录制与回放，今天先跑通 rosbag2 录制，把运行时现场变成可复现证据。
- 录制对象是 `/robot/imu`、`/robot/joint_states` 和 `/runtime/heartbeat`，覆盖传感器输入、关节状态和节点存活证据。
- 已新增 `runtime_rosbag_recording_review`，输出录制计划、`ros2 bag record` / `ros2 bag info` / `ros2 bag play` 命令和面试总结。
- 已新增 `scripts/verify_ros2_bag_recording.sh`，自动启动 demo、录制 3 个 Topic，并用 `ros2 bag info` 验证 Topic 和消息数。
- `verify_ros2_runtime_demo.sh` 已纳入 rosbag recording review 检查，最终 `[ok]` 行包含 `rosbag recording review`。
- 今日复盘目标是把“rosbag2 是问题现场复现能力”讲清楚。

2026-08-19 练习重点：

- 第 6 周周三给运行时日志增加 `session_id`，让一次实验里的日志、心跳、Service 快照和 perf 指标能被串起来。
- 已新增 `runtime_session.hpp`，统一从 `ROBOT_RUNTIME_SESSION_ID` 读取实验 id，未设置时使用 `local_session`。
- `runtime_node` 的 `[runtime_log]`、`/runtime/heartbeat`、`/runtime/query_status` 和 `[perf]` 已输出 `session_id`。
- `sensor_sim_node` 与 `heartbeat_monitor_node` 的 heartbeat 已输出 `session_id`，`watchdog_node` 能解析 heartbeat payload 中的 session 字段。
- 已新增 `runtime_session_trace_review`，输出 session trace surfaces、命令和面试总结。
- `verify_ros2_runtime_demo.sh` 已纳入 session trace review 和 session 字段检查。
- 今日复盘目标是把“session id 把多节点日志和 rosbag2 录制变成同一次实验的证据链”讲清楚。

2026-08-20 练习重点：

- 第 6 周周四写固定故障复现脚本，自动触发 `SensorTimeout -> FAULT -> ResetFault -> RecoveryDone -> STANDBY`。
- 已新增 `runtime_fault_reproduction_review`，输出固定故障路径、证据点和面试总结。
- 已新增 `scripts/verify_fault_reproduction.sh`，自动构建、启动 demo、设置 `ROBOT_RUNTIME_SESSION_ID`、触发故障、验证恢复。
- 脚本会检查 `state=FAULT`、`runtime_error=SENSOR_TIMEOUT`、`runtime_severity=CRITICAL`，并确认恢复后回到 `STANDBY`。
- 脚本同时检查同一 `session_id` 是否出现在 query、runtime log、heartbeat 和 perf 输出中。
- `verify_ros2_runtime_demo.sh` 已纳入 fault reproduction review 检查，最终 `[ok]` 行包含 `fault reproduction review`。
- 今日复盘目标是把“故障复现脚本把异常处理变成可重复验证的工程证据”讲清楚。

2026-08-21 练习重点：

- 第 6 周周五整理数据闭环笔记，把采集、录制、回放、定位、修复和复验连成一条工程链路。
- 已新增 `runtime_data_loop_review`，输出 `capture -> replay -> diagnose -> fix -> verify` 闭环、证据链和面试总结。
- 闭环入口包含 `ros2 bag record`、`ros2 bag info`、`ros2 bag play`、`ROBOT_RUNTIME_SESSION_ID`、`query_status`、`[runtime_log]`、`[perf]` 和 `verify_fault_reproduction.sh`。
- `verify_ros2_runtime_demo.sh` 已纳入 data loop review 检查，确保今日复盘内容可以随 ROS2 主验收一起验证。
- 今日复盘目标是把“rosbag2 保存现场、session id 串联证据、故障脚本复验修复”讲成完整的数据闭环。

2026-08-27 练习重点：

- 第 7 周周四把推理函数接入 ROS2 Runtime Demo，形成传感器输入到推理结果输出的最小链路。
- 已新增 `inference_node`，订阅 `/robot/imu` 和 `/robot/joint_states`。
- 已新增 `/runtime/inference_score` Topic，输出 `model=tiny_robot_score`、`session_id`、`score`、`status` 和输入特征。
- `runtime_demo.launch.py` 已启动 `inference_node`，主验收脚本已检查推理 Topic 和 `[inference]` 日志。
- 今日复盘目标是把“离线推理函数如何进入 ROS2 Topic 链路”讲清楚。

注意：Codex 当前是通过提升权限进入这个 WSL 发行版完成验证的；普通 PowerShell 里如果 `wsl -d Ubuntu` 仍提示找不到发行版，需要在你的普通用户上下文中重新初始化/安装 Ubuntu，或把现有发行版导入普通用户。

## 关键点

- `package.xml` 声明 package 元信息，以及 Topic、Service、Action 与接口生成依赖。
- `CMakeLists.txt` 生成 `ExecuteTask` 接口，编译 runtime、传感器节点与 Action 取消测试客户端。
- `heartbeat_monitor_node.cpp` 展示如何订阅 `/runtime/heartbeat` 并维护节点心跳表，为 watchdog 做准备。
- `srv/ApplyRuntimeEvent.srv` 展示如何定义一个可复盘的状态机事件入口。
- `sensor_sim_node.cpp` 展示 typed Topic publisher，模拟 IMU 和关节状态持续输出数据。
- `runtime_state_machine.hpp` 展示纯 C++ 运行时状态机，便于脱离 ROS2 做快速验证和复盘。
- `runtime_state_machine.hpp` 同时维护错误码语义，统一提供 severity、recoverable、reason 和 recovery_hint。
- `runtime_state_machine_review.cpp` 展示如何把一周成果变成可运行、可检查的复盘输出。
- `runtime_integration_review.cpp` 展示如何把第 1 阶段成果变成可运行、可检查的集成验收摘要。
- `runtime_sensor_qos_review.cpp` 展示传感器 best effort QoS 的实验结论和面试表达。
- `runtime_command_reliability_review.cpp` 展示控制命令 reliable 语义：Service / Action 必须返回可验证的成功、失败、拒绝、状态变化或取消结果。
- `runtime_queue_depth_review.cpp` 展示 Topic queue depth 的工程取舍：小队列保护新鲜度，大队列保留历史但可能放大 backlog 延迟。
- `runtime_qos_interview_review.cpp` 展示第 5 周 QoS 面试表达，把传感器流、心跳、Service 和 Action 的通信选择连成项目讲述。
- `runtime_rosbag_recording_review.cpp` 展示 rosbag2 录制计划，把 live runtime Topic 转成可回放证据。
- `runtime_session_trace_review.cpp` 展示 session id 如何贯穿 runtime log、heartbeat、query_status 和 perf 指标。
- `runtime_fault_reproduction_review.cpp` 展示固定故障复现路径和恢复链证据。
- `runtime_data_loop_review.cpp` 展示采集、录制、回放、定位、修复和复验的数据闭环证据链。
- `inference_node.cpp` 展示如何订阅模拟传感器数据并发布 `/runtime/inference_score` 推理结果。
- `runtime_inference.hpp` 展示推理函数在 ROS2 节点中的稳定封装边界。
- `runtime_session.hpp` 展示如何从 `ROBOT_RUNTIME_SESSION_ID` 给多节点实验统一打 session id。
- `runtime_qos.hpp` 展示如何把 Topic QoS 策略集中命名：传感器流用 best effort，心跳用 reliable。
- `runtime_node.cpp` 展示 typed Topic subscriber、状态机事件适配、健康判断、状态查询/故障复位 Service、故障语义输出，以及可反馈、可取消的 Action server。
- `runtime_node.cpp` 同时输出 `[perf]` 性能聚合指标，用于区分通信延迟、回调耗时、Action 耗时和心跳新鲜度。
- 构建后必须 `source install/setup.bash`，否则当前 shell 找不到新 package。
- `scripts/verify_ros2_runtime_demo.sh` 是验收入口，用来补齐环境检查、状态机 demo、构建、launch 启动、Topic 发布/订阅检查。
- `scripts/verify_ros2_bag_recording.sh` 是 rosbag2 录制验收入口，用来验证 `/robot/imu`、`/robot/joint_states` 和 `/runtime/heartbeat` 能被录入 bag。
- `scripts/verify_fault_reproduction.sh` 是故障复现验收入口，用来验证 `SensorTimeout`、故障元数据、恢复链和 session id 证据。

## ROS2 适合什么应用场景

- 多传感器机器人：相机、雷达、IMU、关节状态分别由不同 node 发布，感知/定位/规划节点订阅这些数据。
- 机器人本体运行时：状态机、心跳、故障检测、复位、任务调度拆成清晰的节点和服务。
- 仿真和实机切换：Gazebo / Isaac Sim 中的模拟数据和真实驱动可以暴露相似 Topic，让上层算法少改代码。
- 分布式系统：ROS2 基于 DDS，天然支持多个进程甚至多台机器之间发现和通信。
- 工程集成：用 launch 管理多节点启动，用 colcon 管理多个 package 的构建，用 rosbag2 记录和回放问题现场。
