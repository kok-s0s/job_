# URDF Two Joint Arm Demo

This project starts week 9 with a small robot model that can be inspected without launching a full simulator. The URDF defines a base link, two arm links, a tool link, two revolute joints, one fixed tool joint, and explicit inertial, collision, axis, and limit fields.

## Verify

```bash
cd projects/urdf_two_joint_arm_demo
bash scripts/verify_urdf_two_joint_arm_demo.sh
```

Expected output:

```txt
[urdf] robot=two_joint_arm links=4 joints=3
[urdf_link] base_link shoulder_link elbow_link tool0 inertial=4 collision=4
[urdf_joint] shoulder_yaw_joint type=revolute parent=base_link child=shoulder_link axis=0,0,1 limit=-1.57..1.57
[urdf_joint] elbow_pitch_joint type=revolute parent=shoulder_link child=elbow_link axis=0,1,0 limit=-1.20..1.20
[urdf_joint] tool_mount_joint type=fixed parent=elbow_link child=tool0
[ok] two-joint URDF links joints limits and XML structure verified
```
