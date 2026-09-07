# URDF Two Joint Arm Demo

对应练习：[2026-09-07：简单两关节机械臂 URDF](/roadmap/daily/2026-09-07)

这是第 9 周“URDF / TF2 / 机器人模型”的起步项目。今天先把机器人模型拆成 link、joint、origin、axis、limit 这些最小概念，并给出一个可检查的两关节机械臂 URDF。

源码目录：

```txt
projects/urdf_two_joint_arm_demo
```

## 当前能力

- `models/two_joint_arm.urdf` 定义 `base_link`、`shoulder_link`、`elbow_link`。
- `shoulder_yaw_joint` 连接 base 到 shoulder，绕 Z 轴转动。
- `elbow_pitch_joint` 连接 shoulder 到 elbow，绕 Y 轴转动。
- 每个 revolute joint 都写明 `origin`、`axis`、`limit`。
- `urdf_structure_check` 用 C++ 检查 link、joint、父子关系、轴和限位字段。

## 验收命令

```bash
cd projects/urdf_two_joint_arm_demo
bash scripts/verify_urdf_two_joint_arm_demo.sh
```

关键输出：

```txt
[urdf] robot=two_joint_arm links=3 joints=2
[urdf_joint] shoulder_yaw_joint parent=base_link child=shoulder_link axis=0,0,1
[urdf_joint] elbow_pitch_joint parent=shoulder_link child=elbow_link axis=0,1,0
[ok] two-joint URDF structure verified
```

## 复盘要点

URDF 的核心不是几何本身，而是把机器人拆成 link 和 joint 的树结构。link 描述刚体，joint 描述父子 link 的连接关系、坐标偏移、运动轴和运动限制。今天这个模型后续可以继续接 TF2、rviz2、传感器 frame 和 MoveIt2 配置。
