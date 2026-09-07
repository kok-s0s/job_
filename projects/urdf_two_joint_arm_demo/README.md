# URDF Two Joint Arm Demo

This project starts week 9 with a small robot model that can be inspected without launching a full simulator. The URDF defines a base link, a shoulder link, an elbow link, and two revolute joints with explicit axes and limits.

## Verify

```bash
cd projects/urdf_two_joint_arm_demo
bash scripts/verify_urdf_two_joint_arm_demo.sh
```

Expected output:

```txt
[urdf] robot=two_joint_arm links=3 joints=2
[urdf_joint] shoulder_yaw_joint parent=base_link child=shoulder_link axis=0,0,1
[urdf_joint] elbow_pitch_joint parent=shoulder_link child=elbow_link axis=0,1,0
[ok] two-joint URDF structure verified
```
