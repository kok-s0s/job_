# Gazebo Model Smoke Demo

This project starts week 10 with a deterministic Gazebo world smoke check. The world file includes `sun`, `ground_plane`, and a small placeholder robot model so the launch contract can be reviewed even when a GUI Gazebo session is not available.

## Verify

```bash
cd projects/gazebo_model_smoke_demo
bash scripts/verify_gazebo_model_smoke_demo.sh
```

Expected output:

```txt
[gazebo_world] world=two_joint_arm_smoke_world includes=sun,ground_plane model=two_joint_arm_placeholder
[gazebo_model] links=base_link,arm_link joint=base_to_arm_fixed pose=0,0,0,0,0,0
[gazebo_launch] command="gazebo worlds/two_joint_arm.world"
[ok] Gazebo model smoke world structure verified
```
