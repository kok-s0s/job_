# URDF Two Joint Arm Demo

对应练习：

- [2026-09-07：简单两关节机械臂 URDF](/roadmap/daily/2026-09-07)
- [2026-09-08：补齐 link、joint、limit 结构](/roadmap/daily/2026-09-08)
- [2026-09-09：TF2 坐标变换复盘](/roadmap/daily/2026-09-09)
- [2026-09-10：模拟传感器挂到 link](/roadmap/daily/2026-09-10)
- [2026-09-11：URDF / TF2 笔记](/roadmap/daily/2026-09-11)

这是第 9 周“URDF / TF2 / 机器人模型”的起步项目。今天先把机器人模型拆成 link、joint、origin、axis、limit 这些最小概念，并给出一个可检查的两关节机械臂 URDF。

源码目录：

```txt
projects/urdf_two_joint_arm_demo
```

## 当前能力

- `models/two_joint_arm.urdf` 定义 `base_link`、`shoulder_link`、`elbow_link` 和 `tool0`。
- `shoulder_yaw_joint` 连接 base 到 shoulder，绕 Z 轴转动。
- `elbow_pitch_joint` 连接 shoulder 到 elbow，绕 Y 轴转动。
- `tool_mount_joint` 用 fixed joint 把末端工具坐标系挂到 `elbow_link`。
- 每个 revolute joint 都写明 `origin`、`axis`、`limit`。
- 每个 link 都补齐 `visual`、`collision`、`inertial`，降低后续接仿真或动力学工具时的结构风险。
- `imu_mount_joint` 将模拟 IMU frame 固定到 `elbow_link`。
- `tf_tree_review` 输出 base、link、tool、sensor frame 的 TF 树和 `tf2_echo` 命令。
- `sensor_frame_mount_check` 检查传感器 frame 是否挂到指定 link。
- `urdf_structure_check` 用 C++ 检查 link、joint、父子关系、轴、限位、collision 和 inertial 字段。

## 验收命令

```bash
cd projects/urdf_two_joint_arm_demo
bash scripts/verify_urdf_two_joint_arm_demo.sh
```

关键输出：

```txt
[urdf] robot=two_joint_arm links=5 joints=4
[urdf_link] base_link shoulder_link elbow_link tool0 imu_link inertial=5 collision=5
[urdf_joint] shoulder_yaw_joint type=revolute parent=base_link child=shoulder_link axis=0,0,1 limit=-1.57..1.57
[urdf_joint] elbow_pitch_joint type=revolute parent=shoulder_link child=elbow_link axis=0,1,0 limit=-1.20..1.20
[urdf_joint] tool_mount_joint type=fixed parent=elbow_link child=tool0
[urdf_joint] imu_mount_joint type=fixed parent=elbow_link child=imu_link origin=0.25,0,0.08
[ok] TF2 frame tree review ready
[ok] simulated sensor frame is mounted on elbow_link
[ok] two-joint URDF links joints limits sensor frame and XML structure verified
```

## 复盘要点

URDF 的核心不是几何本身，而是把机器人拆成 link 和 joint 的树结构。link 描述刚体，joint 描述父子 link 的连接关系、坐标偏移、运动轴和运动限制。`visual` 给人看，`collision` 给碰撞检测用，`inertial` 给仿真和动力学工具用；这些字段补齐后，后续接 TF2、rviz2、传感器 frame 和 MoveIt2 会更顺。
