# SocketCAN vcan Demo

对应练习：
- [2026-08-31：CAN 帧结构与 SocketCAN 基础命令](/roadmap/daily/2026-08-31)

这是第 8 周“SocketCAN / vcan 模拟驱动”的起步项目。今天先不依赖真实 CAN 设备，而是用一个 C++ 程序解析 candump 风格的帧字符串，确认 CAN id、DLC 和 payload 的边界。

源码目录：

```txt
projects/socketcan_vcan_demo
```

## 当前能力

- `can_frame_basics` 解析 `123#1122334455667788` 这类 candump 字符串。
- 输出标准 11-bit CAN id、DLC 和数据字节。
- 输出 SocketCAN / vcan 基础命令清单：`modprobe vcan`、`ip link add`、`ip link set up`、`candump`。
- `scripts/verify_socketcan_vcan_demo.sh` 一条命令完成 CMake 构建和输出检查。

## 验收命令

```bash
cd projects/socketcan_vcan_demo
bash scripts/verify_socketcan_vcan_demo.sh
```

关键输出：

```txt
[socketcan] basic command checklist
[can_frame] raw=123#1122334455667788 id=0x123 dlc=8 data="11 22 33 44 55 66 77 88"
[can_frame] raw=321#AABBCCDD id=0x321 dlc=4 data="AA BB CC DD"
[ok] CAN frame basics verified
```

## 复盘要点

CAN 驱动开发的第一步是先把帧边界讲清楚：id 标识消息语义，DLC 表示 payload 长度，data 承载执行器状态或控制命令。SocketCAN 把 CAN 设备暴露成 Linux 网络接口，后续可以用 `vcan0` 做无硬件环境下的收发练习。
