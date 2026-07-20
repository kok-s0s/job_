# ROS2 Runtime Demo

对应练习：[2026-07-20：ROS2 Workspace 与 C++ Package](/roadmap/daily/2026-07-20)

这是一个最小 ROS2 C++ package，用来验证 workspace、package、node、`colcon build`、`ros2 run` 的完整流程。

源码目录：

```txt
projects/ros2_runtime_demo/src/robot_runtime_demo
```

## 文件结构

```txt
robot_runtime_demo/
├── CMakeLists.txt
├── package.xml
└── src/
    └── runtime_node.cpp
```

## 在 WSL2 / Ubuntu 22.04 / ROS2 Humble 中验证

把本目录作为 workspace 使用：

```bash
cd projects/ros2_runtime_demo
source /opt/ros/humble/setup.bash
colcon build --packages-select robot_runtime_demo
source install/setup.bash
ros2 run robot_runtime_demo runtime_node
```

预期输出每秒出现一次：

```txt
[INFO] [runtime_node]: runtime_node heartbeat
```

## 关键点

- `package.xml` 声明 package 元信息和 `rclcpp` 依赖。
- `CMakeLists.txt` 查找 `ament_cmake` 和 `rclcpp`，编译并安装 `runtime_node`。
- `runtime_node.cpp` 定义一个 `rclcpp::Node`，用 timer 每秒打印 heartbeat。
- 构建后必须 `source install/setup.bash`，否则当前 shell 找不到新 package。
