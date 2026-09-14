# URDF Two Joint Arm Demo

This project covers week 9 with a small robot model that can be inspected without launching a full simulator. The URDF defines a base link, two arm links, a tool link, a simulated IMU link, two revolute joints, fixed mount joints, and explicit inertial, collision, axis, and limit fields.

## Verify

```bash
cd projects/urdf_two_joint_arm_demo
bash scripts/verify_urdf_two_joint_arm_demo.sh
```

Expected output:

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
