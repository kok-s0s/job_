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

echo "[service] /runtime/query_status"
QUERY_SERVICE_LIST_FILE=$(mktemp)
QUERY_SERVICE_TYPE_FILE=$(mktemp)
QUERY_SERVICE_OUTPUT_FILE=$(mktemp)
ros2 service list >"${QUERY_SERVICE_LIST_FILE}" 2>&1
ros2 service type /runtime/query_status >"${QUERY_SERVICE_TYPE_FILE}" 2>&1
timeout 5s ros2 service call /runtime/query_status std_srvs/srv/Trigger "{}" >"${QUERY_SERVICE_OUTPUT_FILE}" 2>&1

echo "[action] /runtime/execute_task"
ACTION_LIST_FILE=$(mktemp)
ACTION_INTERFACE_FILE=$(mktemp)
ACTION_OUTPUT_FILE=$(mktemp)
ACTION_QUERY_OUTPUT_FILE=$(mktemp)
ACTION_REJECT_OUTPUT_FILE=$(mktemp)
ACTION_CANCEL_OUTPUT_FILE=$(mktemp)
ros2 action list -t >"${ACTION_LIST_FILE}" 2>&1
ros2 interface show robot_runtime_demo/action/ExecuteTask >"${ACTION_INTERFACE_FILE}" 2>&1
timeout 10s ros2 action send_goal --feedback \
  /runtime/execute_task \
  robot_runtime_demo/action/ExecuteTask \
  "{target_steps: 10}" >"${ACTION_OUTPUT_FILE}" 2>&1 &
ACTION_PID=$!
sleep 1
timeout 5s ros2 service call \
  /runtime/query_status std_srvs/srv/Trigger "{}" >"${ACTION_QUERY_OUTPUT_FILE}" 2>&1
wait "${ACTION_PID}"
timeout 5s ros2 action send_goal \
  /runtime/execute_task \
  robot_runtime_demo/action/ExecuteTask \
  "{target_steps: 0}" >"${ACTION_REJECT_OUTPUT_FILE}" 2>&1 || true
timeout 10s ros2 run \
  robot_runtime_demo action_cancel_test_client >"${ACTION_CANCEL_OUTPUT_FILE}" 2>&1

sleep 2
cleanup
trap - EXIT

cat "${OUTPUT_FILE}"
cat "${IMU_OUTPUT_FILE}"
cat "${JOINT_OUTPUT_FILE}"
cat "${IMU_HZ_OUTPUT_FILE}"
cat "${JOINT_HZ_OUTPUT_FILE}"
cat "${SERVICE_OUTPUT_FILE}"
cat "${QUERY_SERVICE_LIST_FILE}"
cat "${QUERY_SERVICE_TYPE_FILE}"
cat "${QUERY_SERVICE_OUTPUT_FILE}"
cat "${ACTION_LIST_FILE}"
cat "${ACTION_INTERFACE_FILE}"
cat "${ACTION_OUTPUT_FILE}"
cat "${ACTION_QUERY_OUTPUT_FILE}"
cat "${ACTION_REJECT_OUTPUT_FILE}"
cat "${ACTION_CANCEL_OUTPUT_FILE}"

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

if ! grep -q "runtime status state=" "${OUTPUT_FILE}"; then
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

if ! grep -q "/runtime/query_status" "${QUERY_SERVICE_LIST_FILE}"; then
  echo "query_status service was not listed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  rm -f "${QUERY_SERVICE_LIST_FILE}"
  rm -f "${QUERY_SERVICE_TYPE_FILE}"
  rm -f "${QUERY_SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "std_srvs/srv/Trigger" "${QUERY_SERVICE_TYPE_FILE}"; then
  echo "query_status service type was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  rm -f "${QUERY_SERVICE_LIST_FILE}"
  rm -f "${QUERY_SERVICE_TYPE_FILE}"
  rm -f "${QUERY_SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "success=True" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -Eq "state=(STANDBY|RUNNING)" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -q "runtime_error=NONE" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -q "imu_count=.*joint_count=.*imu_latency_ms=.*joint_latency_ms=.*joint_valid=1" "${QUERY_SERVICE_OUTPUT_FILE}"; then
  echo "query_status service response did not include the expected runtime summary" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  rm -f "${QUERY_SERVICE_LIST_FILE}"
  rm -f "${QUERY_SERVICE_TYPE_FILE}"
  rm -f "${QUERY_SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "/runtime/execute_task.*robot_runtime_demo/action/ExecuteTask" "${ACTION_LIST_FILE}"; then
  echo "execute_task action was not listed with the expected type" >&2
  exit 1
fi

if ! grep -q "target_steps" "${ACTION_INTERFACE_FILE}" ||
  ! grep -q "current_step" "${ACTION_INTERFACE_FILE}" ||
  ! grep -q "progress" "${ACTION_INTERFACE_FILE}"; then
  echo "ExecuteTask action interface did not contain the expected fields" >&2
  exit 1
fi

if ! grep -q "Goal accepted" "${ACTION_OUTPUT_FILE}" ||
  ! grep -q "current_step" "${ACTION_OUTPUT_FILE}" ||
  ! grep -q "progress" "${ACTION_OUTPUT_FILE}" ||
  ! grep -Eq "success[:=][[:space:]]*(true|True)" "${ACTION_OUTPUT_FILE}" ||
  ! grep -q "task completed" "${ACTION_OUTPUT_FILE}"; then
  echo "execute_task normal completion output was not observed" >&2
  exit 1
fi

if ! grep -q "success=True" "${ACTION_QUERY_OUTPUT_FILE}" ||
  ! grep -q "state=RUNNING" "${ACTION_QUERY_OUTPUT_FILE}" ||
  ! grep -q "task_state=RUNNING" "${ACTION_QUERY_OUTPUT_FILE}"; then
  echo "query_status did not respond while execute_task was running" >&2
  exit 1
fi

if ! grep -Eq "Goal (was )?rejected" "${ACTION_REJECT_OUTPUT_FILE}"; then
  echo "execute_task invalid-goal rejection was not observed" >&2
  exit 1
fi

if ! grep -q "cancel_result" "${ACTION_CANCEL_OUTPUT_FILE}" ||
  ! grep -q "success=0" "${ACTION_CANCEL_OUTPUT_FILE}" ||
  ! grep -q "task canceled at step" "${ACTION_CANCEL_OUTPUT_FILE}" ||
  ! grep -Eq "feedback_count=[1-9][0-9]*" "${ACTION_CANCEL_OUTPUT_FILE}"; then
  echo "execute_task cancellation output was not observed" >&2
  exit 1
fi

rm -f "${OUTPUT_FILE}"
rm -f "${IMU_OUTPUT_FILE}"
rm -f "${JOINT_OUTPUT_FILE}"
rm -f "${IMU_HZ_OUTPUT_FILE}"
rm -f "${JOINT_HZ_OUTPUT_FILE}"
rm -f "${SERVICE_OUTPUT_FILE}"
rm -f "${QUERY_SERVICE_LIST_FILE}"
rm -f "${QUERY_SERVICE_TYPE_FILE}"
rm -f "${QUERY_SERVICE_OUTPUT_FILE}"
rm -f "${ACTION_LIST_FILE}"
rm -f "${ACTION_INTERFACE_FILE}"
rm -f "${ACTION_OUTPUT_FILE}"
rm -f "${ACTION_QUERY_OUTPUT_FILE}"
rm -f "${ACTION_REJECT_OUTPUT_FILE}"
rm -f "${ACTION_CANCEL_OUTPUT_FILE}"
echo "[ok] ROS2 runtime demo verified: typed topics, runtime health, query/reset services, and execute_task Action completion/rejection/cancellation are working"
