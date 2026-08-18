#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
PACKAGE_NAME=robot_runtime_demo
LAUNCH_FILE=runtime_demo.launch.py

wait_for_topic() {
  local topic_name=$1
  local attempts=${2:-30}
  for ((i = 0; i < attempts; ++i)); do
    if ros2 topic list 2>/dev/null | grep -qx "${topic_name}"; then
      return 0
    fi
    sleep 0.5
  done
  echo "topic not discovered: ${topic_name}" >&2
  return 1
}

if [[ -z "${ROS_DISTRO:-}" ]]; then
  if [[ -f /opt/ros/jazzy/setup.bash ]]; then
    set +u
    # shellcheck disable=SC1091
    source /opt/ros/jazzy/setup.bash
    set -u
  elif [[ -f /opt/ros/humble/setup.bash ]]; then
    set +u
    # shellcheck disable=SC1091
    source /opt/ros/humble/setup.bash
    set -u
  fi
fi

echo "[check] ros2 command"
command -v ros2

echo "[check] ros2 bag command"
ros2 bag --help >/dev/null

echo "[check] colcon command"
command -v colcon

cd "${WORKSPACE_DIR}"
echo "[build] ${PACKAGE_NAME}"
CMAKE_BUILD_PARALLEL_LEVEL=1 MAKEFLAGS=-j1 colcon build \
  --executor sequential \
  --packages-select "${PACKAGE_NAME}" \
  --event-handlers console_direct+

set +u
# shellcheck disable=SC1091
source "${WORKSPACE_DIR}/install/setup.bash"
set -u

echo "[review] runtime_rosbag_recording_review"
ROSBAG_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_rosbag_recording_review >"${ROSBAG_REVIEW_OUTPUT_FILE}" 2>&1

BAG_ROOT=$(mktemp -d)
BAG_PATH="${BAG_ROOT}/runtime_capture"
RECORD_OUTPUT_FILE=$(mktemp)
BAG_INFO_FILE=$(mktemp)
LAUNCH_OUTPUT_FILE=$(mktemp)

ros2 launch "${PACKAGE_NAME}" "${LAUNCH_FILE}" >"${LAUNCH_OUTPUT_FILE}" 2>&1 &
LAUNCH_PID=$!

cleanup() {
  if kill -0 "${LAUNCH_PID}" >/dev/null 2>&1; then
    kill "${LAUNCH_PID}" >/dev/null 2>&1 || true
    wait "${LAUNCH_PID}" >/dev/null 2>&1 || true
  fi
  rm -rf "${BAG_ROOT}"
  rm -f "${RECORD_OUTPUT_FILE}" "${BAG_INFO_FILE}" "${LAUNCH_OUTPUT_FILE}" "${ROSBAG_REVIEW_OUTPUT_FILE}"
}
trap cleanup EXIT

wait_for_topic /robot/imu
wait_for_topic /robot/joint_states
wait_for_topic /runtime/heartbeat

echo "[bag] record runtime topics"
set +e
timeout 8s ros2 bag record \
  -o "${BAG_PATH}" \
  --topics \
  /robot/imu \
  /robot/joint_states \
  /runtime/heartbeat >"${RECORD_OUTPUT_FILE}" 2>&1
RECORD_STATUS=$?
set -e

if [[ "${RECORD_STATUS}" -ne 0 && "${RECORD_STATUS}" -ne 124 ]]; then
  cat "${RECORD_OUTPUT_FILE}"
  echo "ros2 bag record failed with status ${RECORD_STATUS}" >&2
  exit 1
fi

echo "[bag] info"
ros2 bag info "${BAG_PATH}" >"${BAG_INFO_FILE}" 2>&1

cat "${ROSBAG_REVIEW_OUTPUT_FILE}"
cat "${RECORD_OUTPUT_FILE}"
cat "${BAG_INFO_FILE}"

if ! grep -q "\[ok\] rosbag recording review ready" "${ROSBAG_REVIEW_OUTPUT_FILE}"; then
  echo "runtime_rosbag_recording_review output was not observed" >&2
  exit 1
fi

if ! grep -q "Topic information" "${BAG_INFO_FILE}" ||
  ! grep -q "Topic: /robot/imu" "${BAG_INFO_FILE}" ||
  ! grep -q "Topic: /robot/joint_states" "${BAG_INFO_FILE}" ||
  ! grep -q "Topic: /runtime/heartbeat" "${BAG_INFO_FILE}" ||
  ! grep -Eq "Messages:[[:space:]]*[1-9][0-9]*" "${BAG_INFO_FILE}"; then
  echo "rosbag2 recording did not contain the expected runtime topics" >&2
  exit 1
fi

echo "[ok] rosbag2 runtime recording verified: /robot/imu, /robot/joint_states, and /runtime/heartbeat recorded with messages"
