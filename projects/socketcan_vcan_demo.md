# SocketCAN vcan Demo

对应练习：

- [2026-08-31：CAN 帧结构与 SocketCAN 基础命令](/roadmap/daily/2026-08-31)
- [2026-09-01：vcan 环境收发测试](/roadmap/daily/2026-09-01)
- [2026-09-02：执行器状态接收程序](/roadmap/daily/2026-09-02)
- [2026-09-03：50Hz 周期命令发送程序](/roadmap/daily/2026-09-03)
- [2026-09-04：CAN 接入状态机验收](/roadmap/daily/2026-09-04)

这是第 8 周“SocketCAN / vcan 模拟驱动”的项目。它不依赖真实 CAN 设备，而是用一组 C++ 程序固定 CAN 帧解析、vcan 收发命令、执行器状态解码、周期控制命令和故障映射。

源码目录：

```txt
projects/socketcan_vcan_demo
```

## 当前能力

- `can_frame_basics` 解析 `123#1122334455667788` 这类 candump 字符串，输出标准 11-bit CAN id、DLC 和数据字节。
- `vcan_loopback_demo` 固化 `cansend vcan0 ...` 与 `candump vcan0` 的回环验证路径。
- `actuator_status_receiver` 解析执行器状态帧里的 `position`、`velocity`、`fault` 和 `fault_code`。
- `periodic_command_sender` 生成 50Hz 控制命令帧，明确 20ms 控制周期。
- `can_runtime_fault_bridge` 将 CAN 超时或执行器 fault 位映射为 runtime `FAULT`。
- `scripts/verify_socketcan_vcan_demo.sh` 一条命令完成 CMake 构建和输出检查。

## 验收命令

```bash
cd projects/socketcan_vcan_demo
bash scripts/verify_socketcan_vcan_demo.sh
```

关键输出：

```txt
[can_frame] raw=123#1122334455667788 id=0x123 dlc=8 data="11 22 33 44 55 66 77 88"
[ok] vcan loopback command path verified
[ok] actuator status receiver decoded position velocity and fault
[ok] periodic command sender generated 50Hz control frames
[ok] CAN timeout and actuator fault map to runtime Fault
```

## 复盘要点

CAN 驱动开发的第一步是先把帧边界讲清楚：id 标识消息语义，DLC 表示 payload 长度，data 承载执行器状态或控制命令。SocketCAN 把 CAN 设备暴露成 Linux 网络接口，可以用 `vcan0` 做无硬件环境下的收发练习。落到机器人 runtime 时，CAN 状态帧需要被翻译成结构化状态，控制帧要按固定周期发送，超时和 fault 位必须能触发状态机进入故障态。
