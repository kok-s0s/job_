# 11 Observer 模式（机械臂事件系统）

> OOP 设计模式：Subject / Observer 接口分离，Qt signals/slots 的底层思路

## 与状态机的关系

状态机（08）负责**驱动**状态变化，Observer 负责**响应**状态变化——两者组合后，RobotArm 既能自动执行状态转移，又能向外广播事件，而不需要知道外部有多少个监听者。

## 核心结构

```
Observer<Event>          ← 纯虚接口，只有 on_event()
    ↑ 继承
Logger / SafetyMonitor / Dashboard   ← 具体实现

Subject<Event>           ← 持有 observer 列表，提供 subscribe/unsubscribe/notify
    ↑ 继承
RobotArm                 ← 状态变化时调用 notify(StateChange{...})
```

## observer.hpp 关键设计

```cpp
template<typename Event>
class Observer {
public:
    virtual void on_event(const Event& e) = 0;
    virtual ~Observer() = default;        // 虚析构：通过基类指针删除子类必须有
};

template<typename Event>
class Subject {
protected:
    void notify(const Event& e) {
        for (auto* obs : observers_) obs->on_event(e);
    }
private:
    std::vector<Observer<Event>*> observers_;  // 非拥有指针，调用方管生命周期
};
```

## 三个观察者各司其职

```cpp
class Logger : public Observer<StateChange> {
    void on_event(const StateChange& e) override {
        // 记录所有状态转移
    }
};

class SafetyMonitor : public Observer<StateChange> {
    void on_event(const StateChange& e) override {
        if (e.trigger == ArmEvent::ERROR)  // 只关心故障事件
            // 触发急停
    }
};

class Dashboard : public Observer<StateChange> {
    void on_event(const StateChange& e) override {
        // 刷新 UI 显示当前状态
    }
};
```

## 实际运行输出

```
=== normal pick-and-place ===
--- send event ---
  [Logger]  IDLE → MOVING
  [Dash]    status → MOVING
...

=== unsubscribe dashboard, then error recovery ===
  [Logger]  IDLE → MOVING       ← Dashboard 已退订，不再出现
--- ERROR ---
  [Logger]  MOVING → IDLE
  [Safety]  *** FAULT DETECTED — halting arm ***
```

## 面试常问

**Q：Observer 和 Qt signals/slots 有什么关系？**

Qt 的 signals/slots 是 Observer 模式的工程化实现：signal 对应 `notify()`，slot 对应 `on_event()`，`connect()` 对应 `subscribe()`。Qt 通过 MOC 代码生成实现了类型安全和跨线程（queued connection），本质思路一样。

**Q：Subject 为什么持有裸指针而不是 `shared_ptr`？**

裸指针表达"不拥有"语义——Subject 只是借用 Observer，不负责它的生命周期。如果用 `shared_ptr` 会产生循环引用风险，且会强制所有 Observer 必须用 `make_shared` 创建。用 `weak_ptr` 可以解决循环问题，但会让代码复杂化。

**Q：Observer 为什么需要虚析构？**

如果通过 `Observer<Event>*` 基类指针 `delete` 一个子类对象，没有虚析构会导致只调用基类析构，子类资源泄漏。只要类有虚函数，就应该加虚析构。

**Q：notify 时如果某个 Observer 里调用了 unsubscribe，会发生什么？**

本实现会迭代器失效（遍历 `observers_` 时修改了它）。防御方案：在 `notify` 里先拷贝一份 snapshot 再遍历，或者用标记删除（标记为 null，通知结束后再清理）。
