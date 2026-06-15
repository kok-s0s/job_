# 08 状态机（机械臂场景）

> 练习点：模板类设计、enum class、std::function 回调、unordered_map 编码技巧

## 项目结构

```
projects/state_machine/
└── src/
    ├── state_machine.hpp   # 通用模板
    ├── robot_arm.hpp       # 机械臂具体场景
    └── main.cpp            # 演示
```

## 状态转移图

```
              START         ARRIVED        GRASPED
  IDLE ──────────→ MOVING ──────────→ GRASPING ──────────→ HOLDING
   ↑                  │                   │                    │
   │     ERROR        │      ERROR        │       ERROR        │ RELEASE
   └──────────────────┴───────────────────┴────────────────────┘
   ↑
   │ HOME
RETURNING
```

## 通用状态机模板（state_machine.hpp）

```cpp
template<typename State, typename Event>
class StateMachine {
public:
    using Action = std::function<void()>;

    void on_enter(State s, Action a);        // 进入状态时触发
    void on_exit (State s, Action a);        // 离开状态时触发
    void add(State from, Event ev, State to, // 注册转移规则
             Action action = nullptr);
    bool process(Event ev);                  // 驱动状态机，返回是否触发转移
    State current() const;
};
```

**转移触发顺序：**
```
on_exit(旧状态) → transition action → 切换 current_ → on_enter(新状态)
```

## 关键实现细节

### (State, Event) 编码为单一 key

```cpp
static std::size_t tr_key(State from, Event ev) {
    return (static_cast<std::size_t>(from) << 16) |
            static_cast<std::size_t>(ev);
}
```

enum class 不能直接作为 `unordered_map` 的 key（标准未提供默认 hash），把两个枚举编码成一个 `size_t` 绕过这个限制，同时查找仍是 O(1)。

### ERROR 批量注册

```cpp
for (auto s : {ArmState::MOVING, ArmState::GRASPING,
                ArmState::HOLDING, ArmState::RETURNING}) {
    sm_.add(s, ArmEvent::ERROR, ArmState::IDLE,
            [] { log("!!! EMERGENCY STOP !!!"); });
}
```

紧急停止从任意运动状态都能触发，用循环批量注册而不是逐条写，避免遗漏。

### 无效事件静默忽略

```cpp
bool process(Event ev) {
    auto it = transitions_.find(tr_key(current_, ev));
    if (it == transitions_.end()) return false;  // 不崩溃，只是忽略
    ...
}
```

未注册的 (状态, 事件) 组合直接返回 false，调用方决定是否打日志或报警。

## 实际运行输出

```
=== normal pick-and-place sequence ===

event: START — move to target
  [arm] moving to target position...

event: ARRIVED — at target
  [arm] (motion stopped)
  [arm] closing gripper...

event: GRASPED — force sensor OK
  [arm] object secured

event: RELEASE — drop command
  [arm] returning to home position...

event: HOME — back at origin
  [arm] (motion stopped)
  [arm] waiting for command

=== invalid event (ignored) ===

event: ARRIVED (arm is IDLE, nothing to ignore)
  [ignored] no transition from IDLE for this event

=== error recovery (emergency stop) ===

event: START — move to target
  [arm] moving to target position...

event: ERROR — joint fault detected
  [arm] (motion stopped)
  [arm] !!! EMERGENCY STOP !!!
  [arm] waiting for command

final state: IDLE
```

## 面试常问

**Q：enum class 和 enum 的区别？**

`enum class` 有独立作用域（必须写 `ArmState::IDLE`，不会污染外层命名空间）、不隐式转换为 int、可以指定底层类型（`enum class X : uint8_t`）。现代 C++ 优先用 `enum class`。

**Q：`std::function` 的代价是什么？**

有类型擦除（type erasure）的运行时开销，比直接调用函数指针慢。存 lambda 时若捕获了变量还可能触发堆分配。高频调用路径（如每帧渲染）建议用模板参数或函数指针；状态机回调属于低频路径，`std::function` 完全够用。

**Q：状态机在机器人中的典型应用？**

- 夹爪控制：开 / 关 / 故障
- 导航任务：待机 → 规划 → 运动 → 到达
- 充电管理：正常 → 低电 → 充电中 → 满电
- 安全监控：正常 → 告警 → 急停 → 恢复

**Q：这个实现的局限性？**

不支持层次化状态（HSM，子状态机嵌套）和并行状态。工业场景可以用 `boost::statechart` 或 `sml`（C++14 header-only）。
