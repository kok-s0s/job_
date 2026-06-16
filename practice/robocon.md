# 12 RoboMon — 机械臂仿真控制台

> 落地项目：把前面练过的所有东西组合进一个可交互的终端程序

## 跑起来

```bash
cd projects/robocon
cmake -B build && cmake --build build
./build/robocon
```

## 完整操作流程

```
start          ← 机械臂开始移动，LiDAR 从 1.5m 逐渐缩短
               （等 ~5 秒）
[>>] Target reached. Type grasp.
grasp          ← 夹爪闭合，力传感器从 0 升到 15N
               （等 ~2 秒，自动进入 HOLDING）
[>>] Object secured. Type release.
release        ← 机械臂返回，LiDAR 恢复到 1.5m
               （等 ~5 秒，自动回 IDLE）
[>>] Home reached. Type start.
status         ← 随时查看当前传感器读数
quit
```

## 哪些东西在里面

| 之前练的 | 在这里的体现 |
|--|--|
| 多线程 | `sim thread` 持续运行，与 `main thread` 并发 |
| mutex | `Robot::mtx` 保护所有共享字段 |
| BlockingQueue | `blocking_queue.hpp` 直接复用 |
| State Machine | `Robot::enter()` + switch 驱动工作流 |
| Observer 思路 | `emit()` 在状态变化时主动推送消息到终端 |
| ANSI 终端 | `\r\033[K` 覆写提示行，颜色区分状态 |

## 架构图

```
main thread ──────────── getline ──→ handle(cmd, robot)
                                         │
                                    robot.mtx 加锁
                                    state machine 转移
                                    emit() 输出
                                         │
sim thread  ── 600ms tick ──────→ robot.mtx 加锁
                                    更新 lidar / force
                                    检查自动转移条件
                                    emit() 输出传感器读数
```

## 关键代码片段

### 传感器仿真（MOVING 状态下 LiDAR 逼近目标）

```cpp
case State::MOVING:
    robot.lidar = std::max(0.50f, 1.50f - t * 0.20f);
    if (t > 5.0f && !robot.hinted) {
        robot.hinted = true;
        emit("[>>] Target reached. Type grasp.");
    }
    break;
```

### 命令处理（持锁，构造输出，锁外打印）

```cpp
{
    std::lock_guard lock(robot.mtx);
    robot.enter(State::MOVING);
    out = "[" + ts() + "] Arm moving...";
}
emit(out);   // 锁已释放，安全打印
```

## 可以继续扩展的方向

- 加 TCP 控制端口：`nc localhost 9999` 远程发命令（复用 tcp_chat_server 的代码）
- 加 Qt GUI：把 `emit()` 换成 Qt signal，实时更新界面
- 加 ROS2 node：把状态变化发布成 topic，在 RViz 里可视化
