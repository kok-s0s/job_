# Gazebo Model Smoke Demo

对应练习：[2026-09-14：Gazebo 模型冒烟启动](/roadmap/daily/2026-09-14)

这是第 10 周“Gazebo / MoveIt2 入门集成”的起步项目。今天先把 Gazebo world 的最小合同固定下来：world 能包含光源、地面和机器人模型，模型有 link、joint 和 pose，后续再替换成更完整的 URDF/ros2_control 配置。

源码目录：

```txt
projects/gazebo_model_smoke_demo
```

## 当前能力

- `worlds/two_joint_arm.world` 定义 `two_joint_arm_smoke_world`。
- world 包含 `model://sun` 和 `model://ground_plane`。
- world 内放置 `two_joint_arm_placeholder`，用于验证模型进入仿真场景的结构。
- `gazebo_world_check` 检查 SDF version、world、include、model、link、joint 和 launch 命令。
- 如果环境安装了 `gz`，验证脚本会额外运行 `gz sdf -k`。

## 验收命令

```bash
cd projects/gazebo_model_smoke_demo
bash scripts/verify_gazebo_model_smoke_demo.sh
```

关键输出：

```txt
[gazebo_world] world=two_joint_arm_smoke_world includes=sun,ground_plane model=two_joint_arm_placeholder
[gazebo_launch] command="gazebo worlds/two_joint_arm.world"
[ok] Gazebo model smoke world structure verified
```

## 复盘要点

Gazebo 接入的第一步不是复杂控制，而是确认 world、模型资源、pose 和基础 link/joint 都能被工具链识别。没有 GUI 环境时，先把 SDF/URDF 文件和命令合同做成脚本化检查，后续换到装有 Gazebo 的机器上再启动可视化验证。
