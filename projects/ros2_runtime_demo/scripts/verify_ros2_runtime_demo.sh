#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
PACKAGE_NAME=robot_runtime_demo
LAUNCH_FILE=runtime_demo.launch.py

if [[ -z "${ROS_DISTRO:-}" ]]; then
  if [[ -f /opt/ros/jazzy/setup.bash ]]; then
    # Keep the script friendly for a fresh Ubuntu 24.04 shell.
    set +u
    # shellcheck disable=SC1091
    source /opt/ros/jazzy/setup.bash
    set -u
  elif [[ -f /opt/ros/humble/setup.bash ]]; then
    # Keep the script friendly for a fresh Ubuntu 22.04 shell.
    set +u
    # shellcheck disable=SC1091
    source /opt/ros/humble/setup.bash
    set -u
  fi
fi

echo "[check] ros2 command"
command -v ros2

echo "[check] ROS_DISTRO=${ROS_DISTRO:-<unset>}"
if [[ "${ROS_DISTRO:-}" != "humble" && "${ROS_DISTRO:-}" != "jazzy" ]]; then
  echo "Expected ROS_DISTRO=humble or jazzy. Run one of:" >&2
  echo "  source /opt/ros/humble/setup.bash" >&2
  echo "  source /opt/ros/jazzy/setup.bash" >&2
  exit 1
fi

echo "[check] ros2 help"
ros2 --help >/dev/null

echo "[check] colcon command"
command -v colcon

echo "[build] ${PACKAGE_NAME}"
cd "${WORKSPACE_DIR}"
colcon build --packages-select "${PACKAGE_NAME}" --event-handlers console_direct+

# shellcheck disable=SC1091
set +u
source "${WORKSPACE_DIR}/install/setup.bash"
set -u

echo "[run] ${PACKAGE_NAME}/${LAUNCH_FILE}"
OUTPUT_FILE=$(mktemp)

ros2 launch "${PACKAGE_NAME}" "${LAUNCH_FILE}" >"${OUTPUT_FILE}" 2>&1 &
LAUNCH_PID=$!

cleanup() {
  if kill -0 "${LAUNCH_PID}" >/dev/null 2>&1; then
    kill "${LAUNCH_PID}" >/dev/null 2>&1 || true
    wait "${LAUNCH_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

sleep 4

echo "[topic] /robot/imu --once"
IMU_OUTPUT_FILE=$(mktemp)
timeout 5s ros2 topic echo /robot/imu --once >"${IMU_OUTPUT_FILE}" 2>&1

echo "[topic] /robot/joint_states --once"
JOINT_OUTPUT_FILE=$(mktemp)
timeout 5s ros2 topic echo /robot/joint_states --once >"${JOINT_OUTPUT_FILE}" 2>&1

echo "[hz] /robot/imu"
IMU_HZ_OUTPUT_FILE=$(mktemp)
timeout 8s ros2 topic hz /robot/imu >"${IMU_HZ_OUTPUT_FILE}" 2>&1 || true

echo "[hz] /robot/joint_states"
JOINT_HZ_OUTPUT_FILE=$(mktemp)
timeout 8s ros2 topic hz /robot/joint_states >"${JOINT_HZ_OUTPUT_FILE}" 2>&1 || true

echo "[service] /runtime/reset_fault"
SERVICE_OUTPUT_FILE=$(mktemp)
timeout 5s ros2 service call /runtime/reset_fault std_srvs/srv/Trigger "{}" >"${SERVICE_OUTPUT_FILE}" 2>&1

sleep 2
cleanup
trap - EXIT

cat "${OUTPUT_FILE}"
cat "${IMU_OUTPUT_FILE}"
cat "${JOINT_OUTPUT_FILE}"
cat "${IMU_HZ_OUTPUT_FILE}"
cat "${JOINT_HZ_OUTPUT_FILE}"
cat "${SERVICE_OUTPUT_FILE}"

if ! grep -q "published sensors" "${OUTPUT_FILE}"; then
  echo "typed sensor publisher output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "runtime status=" "${OUTPUT_FILE}"; then
  echo "runtime subscriber output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "imu_latency_ms=.*joint_latency_ms=.*joint_valid=1" "${OUTPUT_FILE}"; then
  echo "runtime subscriber health output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "linear_acceleration" "${IMU_OUTPUT_FILE}"; then
  echo "/robot/imu output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "joint_1" "${JOINT_OUTPUT_FILE}"; then
  echo "/robot/joint_states output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "average rate" "${IMU_HZ_OUTPUT_FILE}"; then
  echo "/robot/imu hz output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "average rate" "${JOINT_HZ_OUTPUT_FILE}"; then
  echo "/robot/joint_states hz output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "success=True" "${SERVICE_OUTPUT_FILE}"; then
  echo "reset service output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

rm -f "${OUTPUT_FILE}"
rm -f "${IMU_OUTPUT_FILE}"
rm -f "${JOINT_OUTPUT_FILE}"
rm -f "${IMU_HZ_OUTPUT_FILE}"
rm -f "${JOINT_HZ_OUTPUT_FILE}"
rm -f "${SERVICE_OUTPUT_FILE}"
echo "[ok] ROS2 runtime demo verified: typed sensor publishers, runtime health subscriber, topic hz, service call, and launch startup are working"
