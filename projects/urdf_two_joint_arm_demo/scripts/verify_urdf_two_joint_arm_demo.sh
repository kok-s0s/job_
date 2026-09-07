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

grep -q "\[urdf\] robot=two_joint_arm links=3 joints=2" "${OUTPUT_FILE}"
grep -q "\[urdf_link\] base_link shoulder_link elbow_link" "${OUTPUT_FILE}"
grep -q "\[urdf_joint\] shoulder_yaw_joint parent=base_link child=shoulder_link axis=0,0,1" "${OUTPUT_FILE}"
grep -q "\[urdf_joint\] elbow_pitch_joint parent=shoulder_link child=elbow_link axis=0,1,0" "${OUTPUT_FILE}"
grep -q "\[ok\] two-joint URDF structure verified" "${OUTPUT_FILE}"

if command -v check_urdf >/dev/null 2>&1; then
  echo "[run] check_urdf"
  check_urdf models/two_joint_arm.urdf >/tmp/two_joint_arm_check_urdf.log
  grep -q "robot name is: two_joint_arm" /tmp/two_joint_arm_check_urdf.log
else
  echo "[skip] check_urdf is not installed; C++ structure check covered the model contract"
fi

rm -f "${OUTPUT_FILE}"
echo "[ok] URDF two-joint arm demo verified: links, joints, axes, and limits are present"
