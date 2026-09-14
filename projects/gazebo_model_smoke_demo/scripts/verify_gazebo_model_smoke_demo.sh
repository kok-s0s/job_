#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

cd "${PROJECT_DIR}"

echo "[build] gazebo_world_check"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo "[run] gazebo_world_check"
OUTPUT_FILE=$(mktemp)
./build/gazebo_world_check worlds/two_joint_arm.world | tee "${OUTPUT_FILE}"

grep -q "\[gazebo_world\] world=two_joint_arm_smoke_world includes=sun,ground_plane model=two_joint_arm_placeholder" "${OUTPUT_FILE}"
grep -q "\[gazebo_model\] links=base_link,arm_link joint=base_to_arm_fixed pose=0,0,0,0,0,0" "${OUTPUT_FILE}"
grep -q "gazebo worlds/two_joint_arm.world" "${OUTPUT_FILE}"
grep -q "\[ok\] Gazebo model smoke world structure verified" "${OUTPUT_FILE}"

if command -v gz >/dev/null 2>&1; then
  echo "[run] gz sdf -k"
  gz sdf -k worlds/two_joint_arm.world >/tmp/two_joint_arm_world_gz_sdf.log
else
  echo "[skip] gz is not installed; deterministic SDF structure check covered today's smoke contract"
fi

rm -f "${OUTPUT_FILE}"
echo "[ok] Gazebo model smoke demo verified: world, includes, model, links, joint, and launch command are present"
