#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

cd "${PROJECT_DIR}"

echo "[build] urdf_structure_check"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo "[run] urdf_structure_check"
OUTPUT_FILE=$(mktemp)
./build/urdf_structure_check models/two_joint_arm.urdf | tee "${OUTPUT_FILE}"

grep -q "\[urdf\] robot=two_joint_arm links=5 joints=4" "${OUTPUT_FILE}"
grep -q "\[urdf_link\] base_link shoulder_link elbow_link tool0 imu_link inertial=5 collision=5" "${OUTPUT_FILE}"
grep -q "\[urdf_joint\] shoulder_yaw_joint type=revolute parent=base_link child=shoulder_link axis=0,0,1 limit=-1.57..1.57" "${OUTPUT_FILE}"
grep -q "\[urdf_joint\] elbow_pitch_joint type=revolute parent=shoulder_link child=elbow_link axis=0,1,0 limit=-1.20..1.20" "${OUTPUT_FILE}"
grep -q "\[urdf_joint\] tool_mount_joint type=fixed parent=elbow_link child=tool0" "${OUTPUT_FILE}"
grep -q "\[urdf_joint\] imu_mount_joint type=fixed parent=elbow_link child=imu_link origin=0.25,0,0.08" "${OUTPUT_FILE}"
grep -q "\[ok\] two-joint URDF links joints limits sensor frame and XML structure verified" "${OUTPUT_FILE}"

echo "[run] tf_tree_review"
./build/tf_tree_review | tee -a "${OUTPUT_FILE}"

grep -q "\[tf_tree\] root=base_link frames=5 edges=4" "${OUTPUT_FILE}"
grep -q "\[tf_edge\] parent=elbow_link child=imu_link xyz=\"0.25 0 0.08\" rpy=\"0 0 0\"" "${OUTPUT_FILE}"
grep -q "ros2 run tf2_ros tf2_echo base_link tool0" "${OUTPUT_FILE}"
grep -q "\[ok\] TF2 frame tree review ready" "${OUTPUT_FILE}"

echo "[run] sensor_frame_mount_check"
./build/sensor_frame_mount_check models/two_joint_arm.urdf | tee -a "${OUTPUT_FILE}"

grep -q "\[sensor_frame\] frame=imu_link parent=elbow_link joint=imu_mount_joint xyz=0.25,0,0.08" "${OUTPUT_FILE}"
grep -q "\[rviz\] fixed_frame=base_link display=RobotModel+TF sensor_frame=imu_link" "${OUTPUT_FILE}"
grep -q "\[ok\] simulated sensor frame is mounted on elbow_link" "${OUTPUT_FILE}"

if command -v check_urdf >/dev/null 2>&1; then
  echo "[run] check_urdf"
  check_urdf models/two_joint_arm.urdf >/tmp/two_joint_arm_check_urdf.log
  grep -q "robot name is: two_joint_arm" /tmp/two_joint_arm_check_urdf.log
else
  echo "[skip] check_urdf is not installed; C++ structure check covered the model contract"
fi

rm -f "${OUTPUT_FILE}"
echo "[ok] URDF two-joint arm demo verified: XML, links, joints, axes, limits, inertial, collision, TF tree, and sensor frame are present"
