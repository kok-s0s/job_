#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
PACKAGE_NAME=robot_runtime_demo
LAUNCH_FILE=runtime_demo.launch.py

wait_for_service() {
  local service_name=$1
  local attempts=${2:-20}
  for ((i = 0; i < attempts; ++i)); do
    if ros2 service list 2>/dev/null | grep -qx "${service_name}"; then
      return 0
    fi
    sleep 0.5
  done
  echo "service not discovered: ${service_name}" >&2
  return 1
}

wait_for_action() {
  local action_name=$1
  local attempts=${2:-20}
  for ((i = 0; i < attempts; ++i)); do
    if ros2 action list 2>/dev/null | grep -qx "${action_name}"; then
      return 0
    fi
    sleep 0.5
  done
  echo "action not discovered: ${action_name}" >&2
  return 1
}

wait_for_topic() {
  local topic_name=$1
  local attempts=${2:-20}
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
CMAKE_BUILD_PARALLEL_LEVEL=1 MAKEFLAGS=-j1 colcon build \
  --executor sequential \
  --packages-select "${PACKAGE_NAME}" \
  --event-handlers console_direct+

# shellcheck disable=SC1091
set +u
source "${WORKSPACE_DIR}/install/setup.bash"
set -u

echo "[state-machine] runtime_state_machine_demo"
STATE_MACHINE_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_state_machine_demo >"${STATE_MACHINE_OUTPUT_FILE}" 2>&1

echo "[review] runtime_state_machine_review"
STATE_MACHINE_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_state_machine_review >"${STATE_MACHINE_REVIEW_OUTPUT_FILE}" 2>&1

echo "[review] runtime_integration_review"
INTEGRATION_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_integration_review >"${INTEGRATION_REVIEW_OUTPUT_FILE}" 2>&1

echo "[review] runtime_sensor_qos_review"
SENSOR_QOS_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_sensor_qos_review >"${SENSOR_QOS_REVIEW_OUTPUT_FILE}" 2>&1

echo "[review] runtime_command_reliability_review"
COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_command_reliability_review >"${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}" 2>&1

echo "[review] runtime_queue_depth_review"
QUEUE_DEPTH_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_queue_depth_review >"${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" 2>&1

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
wait_for_topic /robot/imu
wait_for_topic /robot/joint_states
wait_for_topic /runtime/heartbeat
wait_for_service /runtime/reset_fault
wait_for_service /runtime/query_status
wait_for_service /runtime/apply_event
wait_for_action /runtime/execute_task

echo "[topic] /robot/imu --once"
IMU_OUTPUT_FILE=$(mktemp)
timeout 10s ros2 topic echo \
  /robot/imu \
  --once \
  --qos-reliability best_effort >"${IMU_OUTPUT_FILE}" 2>&1 || true

echo "[topic] /robot/imu reliable mismatch"
IMU_RELIABLE_MISMATCH_OUTPUT_FILE=$(mktemp)
timeout 5s ros2 topic echo \
  /robot/imu \
  --once \
  --qos-reliability reliable >"${IMU_RELIABLE_MISMATCH_OUTPUT_FILE}" 2>&1 || true

echo "[topic] /robot/joint_states --once"
JOINT_OUTPUT_FILE=$(mktemp)
timeout 10s ros2 topic echo \
  /robot/joint_states \
  --once \
  --qos-reliability best_effort >"${JOINT_OUTPUT_FILE}" 2>&1 || true

echo "[topic] /runtime/heartbeat --once"
HEARTBEAT_OUTPUT_FILE=$(mktemp)
timeout 10s ros2 topic echo /runtime/heartbeat --once >"${HEARTBEAT_OUTPUT_FILE}" 2>&1 || true

echo "[qos] /robot/imu"
IMU_HZ_OUTPUT_FILE=$(mktemp)
ros2 topic info /robot/imu --verbose >"${IMU_HZ_OUTPUT_FILE}" 2>&1 || true

echo "[qos] /robot/joint_states"
JOINT_HZ_OUTPUT_FILE=$(mktemp)
ros2 topic info /robot/joint_states --verbose >"${JOINT_HZ_OUTPUT_FILE}" 2>&1 || true

echo "[watchdog] heartbeat timeout check"
WATCHDOG_OUTPUT_FILE=$(mktemp)
timeout 10s ros2 topic echo /runtime/heartbeat --once >"${WATCHDOG_OUTPUT_FILE}" 2>&1 || true

echo "[service] /runtime/reset_fault"
SERVICE_OUTPUT_FILE=$(mktemp)
timeout 10s ros2 service call /runtime/reset_fault std_srvs/srv/Trigger "{}" >"${SERVICE_OUTPUT_FILE}" 2>&1 || true

echo "[service] /runtime/query_status"
QUERY_SERVICE_LIST_FILE=$(mktemp)
QUERY_SERVICE_TYPE_FILE=$(mktemp)
QUERY_SERVICE_OUTPUT_FILE=$(mktemp)
APPLY_EVENT_TYPE_FILE=$(mktemp)
APPLY_EVENT_TIMEOUT_FILE=$(mktemp)
APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE=$(mktemp)
APPLY_EVENT_UNKNOWN_FILE=$(mktemp)
APPLY_EVENT_RESET_FILE=$(mktemp)
APPLY_EVENT_RECOVERY_FILE=$(mktemp)
APPLY_EVENT_QUERY_OUTPUT_FILE=$(mktemp)
ros2 service list >"${QUERY_SERVICE_LIST_FILE}" 2>&1
ros2 service type /runtime/query_status >"${QUERY_SERVICE_TYPE_FILE}" 2>&1 || true
timeout 10s ros2 service call /runtime/query_status std_srvs/srv/Trigger "{}" >"${QUERY_SERVICE_OUTPUT_FILE}" 2>&1 || true

echo "[service] /runtime/apply_event"
ros2 service type /runtime/apply_event >"${APPLY_EVENT_TYPE_FILE}" 2>&1 || true
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: SensorTimeout}" >"${APPLY_EVENT_TIMEOUT_FILE}" 2>&1 || true
timeout 10s ros2 service call \
  /runtime/query_status std_srvs/srv/Trigger "{}" >"${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}" 2>&1 || true
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: NoSuchEvent}" >"${APPLY_EVENT_UNKNOWN_FILE}" 2>&1 || true
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: ResetFault}" >"${APPLY_EVENT_RESET_FILE}" 2>&1 || true
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: RecoveryDone}" >"${APPLY_EVENT_RECOVERY_FILE}" 2>&1 || true
timeout 10s ros2 service call \
  /runtime/query_status std_srvs/srv/Trigger "{}" >"${APPLY_EVENT_QUERY_OUTPUT_FILE}" 2>&1 || true

echo "[action] /runtime/execute_task"
ACTION_LIST_FILE=$(mktemp)
ACTION_INTERFACE_FILE=$(mktemp)
ACTION_OUTPUT_FILE=$(mktemp)
ACTION_QUERY_OUTPUT_FILE=$(mktemp)
ACTION_REJECT_OUTPUT_FILE=$(mktemp)
ACTION_CANCEL_OUTPUT_FILE=$(mktemp)
ros2 action list -t >"${ACTION_LIST_FILE}" 2>&1 || true
ros2 interface show robot_runtime_demo/action/ExecuteTask >"${ACTION_INTERFACE_FILE}" 2>&1 || true
timeout 15s ros2 action send_goal --feedback \
  /runtime/execute_task \
  robot_runtime_demo/action/ExecuteTask \
  "{target_steps: 30}" >"${ACTION_OUTPUT_FILE}" 2>&1 &
ACTION_PID=$!
for ((i = 0; i < 10; ++i)); do
  timeout 10s ros2 service call \
    /runtime/query_status std_srvs/srv/Trigger "{}" >"${ACTION_QUERY_OUTPUT_FILE}" 2>&1 || true
  if grep -q "state=RUNNING" "${ACTION_QUERY_OUTPUT_FILE}" &&
    grep -q "task_state=RUNNING" "${ACTION_QUERY_OUTPUT_FILE}"; then
    break
  fi
  sleep 0.5
done
wait "${ACTION_PID}" || true
timeout 10s ros2 action send_goal \
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
cat "${HEARTBEAT_OUTPUT_FILE}"
cat "${IMU_HZ_OUTPUT_FILE}"
cat "${JOINT_HZ_OUTPUT_FILE}"
cat "${WATCHDOG_OUTPUT_FILE}"
cat "${SERVICE_OUTPUT_FILE}"
cat "${QUERY_SERVICE_LIST_FILE}"
cat "${QUERY_SERVICE_TYPE_FILE}"
cat "${QUERY_SERVICE_OUTPUT_FILE}"
cat "${APPLY_EVENT_TYPE_FILE}"
cat "${APPLY_EVENT_TIMEOUT_FILE}"
cat "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}"
cat "${APPLY_EVENT_UNKNOWN_FILE}"
cat "${APPLY_EVENT_RESET_FILE}"
cat "${APPLY_EVENT_RECOVERY_FILE}"
cat "${APPLY_EVENT_QUERY_OUTPUT_FILE}"
cat "${ACTION_LIST_FILE}"
cat "${ACTION_INTERFACE_FILE}"
cat "${ACTION_OUTPUT_FILE}"
cat "${ACTION_QUERY_OUTPUT_FILE}"
cat "${ACTION_REJECT_OUTPUT_FILE}"
cat "${ACTION_CANCEL_OUTPUT_FILE}"
cat "${STATE_MACHINE_OUTPUT_FILE}"
cat "${STATE_MACHINE_REVIEW_OUTPUT_FILE}"
cat "${INTEGRATION_REVIEW_OUTPUT_FILE}"
cat "${SENSOR_QOS_REVIEW_OUTPUT_FILE}"
cat "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}"
cat "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}"
cat "${IMU_RELIABLE_MISMATCH_OUTPUT_FILE}"

if ! grep -q "\[ok\] runtime state machine transitions verified" "${STATE_MACHINE_OUTPUT_FILE}"; then
  echo "runtime_state_machine_demo output was not observed" >&2
  rm -f "${STATE_MACHINE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[ok\] week 3 runtime state machine review ready" "${STATE_MACHINE_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "SensorTimeout" "${STATE_MACHINE_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "severity=CRITICAL" "${STATE_MACHINE_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "External callers send events, not target states" "${STATE_MACHINE_REVIEW_OUTPUT_FILE}"; then
  echo "runtime_state_machine_review output was not observed" >&2
  rm -f "${STATE_MACHINE_REVIEW_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[ok\] phase 1 ROS2 runtime integration review ready" "${INTEGRATION_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "phase 1 ROS2 runtime topology" "${INTEGRATION_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "sensor_sim_node" "${INTEGRATION_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "/runtime/execute_task" "${INTEGRATION_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "phase 1 evidence checklist" "${INTEGRATION_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "next week QoS entry questions" "${INTEGRATION_REVIEW_OUTPUT_FILE}"; then
  echo "runtime_integration_review output was not observed" >&2
  rm -f "${INTEGRATION_REVIEW_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[ok\] sensor best-effort QoS review ready" "${SENSOR_QOS_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "/robot/imu" "${SENSOR_QOS_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "best_effort + volatile + keep_last(5)" "${SENSOR_QOS_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "reliable subscriber may not match a best_effort publisher" "${SENSOR_QOS_REVIEW_OUTPUT_FILE}"; then
  echo "runtime_sensor_qos_review output was not observed" >&2
  rm -f "${SENSOR_QOS_REVIEW_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[ok\] command reliability review ready" "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "/runtime/query_status" "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "/runtime/apply_event" "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "/runtime/execute_task" "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "accepted=true means the event name is known" "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "control commands require deterministic acknowledgement" "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}"; then
  echo "runtime_command_reliability_review output was not observed" >&2
  rm -f "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[ok\] queue depth review ready" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "/robot/imu" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "keep_last(5)" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "depth=1" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "depth=20" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "input_rate > processing_rate" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}" ||
  ! grep -q "latency-versus-history tradeoff" "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}"; then
  echo "runtime_queue_depth_review output was not observed" >&2
  rm -f "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}"
  exit 1
fi

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

if ! grep -q "\[qos\] topic=/robot/imu role=sensor_stream reliability=best_effort durability=volatile history=keep_last depth=5" "${OUTPUT_FILE}" ||
  ! grep -q "\[qos\] topic=/robot/joint_states role=sensor_stream reliability=best_effort durability=volatile history=keep_last depth=5" "${OUTPUT_FILE}" ||
  ! grep -q "\[qos\] topic=/runtime/heartbeat role=heartbeat reliability=reliable durability=volatile history=keep_last depth=3" "${OUTPUT_FILE}"; then
  echo "QoS configuration output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[runtime_log\].*event=heartbeat.*message=\"runtime status state=" "${OUTPUT_FILE}"; then
  echo "runtime subscriber output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[runtime_log\].*node=runtime_node.*level=INFO.*event=heartbeat.*state=.*runtime_error=.*severity=.*recoverable=.*latency_ms=.*duration_ms=.*message=" "${OUTPUT_FILE}" ||
  ! grep -q "\[runtime_log\].*event=state_transition.*runtime transition" "${OUTPUT_FILE}" ||
  ! grep -q "\[runtime_log\].*event=service_call.*query_status" "${OUTPUT_FILE}" ||
  ! grep -q "\[runtime_log\].*event=action_feedback.*execute_task feedback" "${OUTPUT_FILE}"; then
  echo "structured runtime_log fields were not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[perf\] node=runtime_node.*imu_latency_ms=.*joint_latency_ms=.*max_callback_duration_ms=.*heartbeat_duration_ms=.*last_action_duration_ms=.*task_step=" "${OUTPUT_FILE}"; then
  echo "runtime performance summary output was not observed" >&2
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

if grep -q "linear_acceleration" "${IMU_RELIABLE_MISMATCH_OUTPUT_FILE}"; then
  echo "reliable subscriber unexpectedly received best-effort IMU data" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${IMU_RELIABLE_MISMATCH_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "node=.*seq=.*stamp_ms=.*status=.*message=" "${HEARTBEAT_OUTPUT_FILE}"; then
  echo "/runtime/heartbeat output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${HEARTBEAT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[heartbeat\] node=sensor_sim_node" "${OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat\] node=runtime_node" "${OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat\] node=heartbeat_monitor_node" "${OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat_rx\] node=runtime_node" "${OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat_table\] node=sensor_sim_node" "${OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat_table\] node=runtime_node" "${OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat_table\] node=heartbeat_monitor_node" "${OUTPUT_FILE}"; then
  echo "heartbeat publisher/subscriber output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${HEARTBEAT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "\[watchdog\] node=sensor_sim_node status=OK" "${OUTPUT_FILE}" ||
  ! grep -q "\[watchdog\] node=runtime_node status=OK" "${OUTPUT_FILE}" ||
  ! grep -q "\[watchdog\] node=heartbeat_monitor_node status=OK" "${OUTPUT_FILE}"; then
  echo "watchdog heartbeat timeout check output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${HEARTBEAT_OUTPUT_FILE}"
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

if ! grep -q "Reliability: BEST_EFFORT" "${IMU_HZ_OUTPUT_FILE}" ||
  ! grep -q "Durability: VOLATILE" "${IMU_HZ_OUTPUT_FILE}"; then
  echo "/robot/imu best-effort QoS output was not observed" >&2
  rm -f "${OUTPUT_FILE}"
  rm -f "${IMU_OUTPUT_FILE}"
  rm -f "${JOINT_OUTPUT_FILE}"
  rm -f "${IMU_HZ_OUTPUT_FILE}"
  rm -f "${JOINT_HZ_OUTPUT_FILE}"
  rm -f "${SERVICE_OUTPUT_FILE}"
  exit 1
fi

if ! grep -q "Reliability: BEST_EFFORT" "${JOINT_HZ_OUTPUT_FILE}" ||
  ! grep -q "Durability: VOLATILE" "${JOINT_HZ_OUTPUT_FILE}"; then
  echo "/robot/joint_states best-effort QoS output was not observed" >&2
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

if ! grep -q "/runtime/apply_event" "${QUERY_SERVICE_LIST_FILE}"; then
  echo "apply_event service was not listed" >&2
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

if ! grep -q "robot_runtime_demo/srv/ApplyRuntimeEvent" "${APPLY_EVENT_TYPE_FILE}"; then
  echo "apply_event service type was not observed" >&2
  exit 1
fi

if ! grep -q "success=True" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -q "runtime_error=NONE" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -q "runtime_severity=INFO" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -q "runtime_recoverable=1" "${QUERY_SERVICE_OUTPUT_FILE}" ||
  ! grep -q "imu_count=.*joint_count=.*imu_latency_ms=.*joint_latency_ms=.*joint_valid=1.*max_callback_duration_ms=.*last_action_duration_ms=" "${QUERY_SERVICE_OUTPUT_FILE}"; then
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

if ! grep -q "state=STANDBY" "${QUERY_SERVICE_OUTPUT_FILE}" &&
  ! grep -q "state=RUNNING" "${QUERY_SERVICE_OUTPUT_FILE}"; then
  echo "query_status service response did not include STANDBY or RUNNING state" >&2
  exit 1
fi

if ! grep -Eq "accepted[:=][[:space:]]*(true|True)" "${APPLY_EVENT_TIMEOUT_FILE}" ||
  ! grep -Eq "transitioned[:=][[:space:]]*(true|True)" "${APPLY_EVENT_TIMEOUT_FILE}" ||
  ! grep -q "current_state='FAULT'" "${APPLY_EVENT_TIMEOUT_FILE}" ||
  ! grep -q "runtime_error='SENSOR_TIMEOUT'" "${APPLY_EVENT_TIMEOUT_FILE}" ||
  ! grep -q "severity=CRITICAL" "${APPLY_EVENT_TIMEOUT_FILE}" ||
  ! grep -q "recovery_hint=check sensor heartbeat then reset fault" "${APPLY_EVENT_TIMEOUT_FILE}"; then
  echo "apply_event SensorTimeout did not enter FAULT" >&2
  exit 1
fi

if ! grep -q "success=False" "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "state=FAULT" "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_error=SENSOR_TIMEOUT" "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_severity=CRITICAL" "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_recoverable=1" "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_recovery_hint=check sensor heartbeat then reset fault" "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}"; then
  echo "query_status did not include expected fault severity metadata" >&2
  exit 1
fi

if ! grep -Eq "accepted[:=][[:space:]]*(false|False)" "${APPLY_EVENT_UNKNOWN_FILE}" ||
  ! grep -q "unknown runtime event: NoSuchEvent" "${APPLY_EVENT_UNKNOWN_FILE}"; then
  echo "apply_event unknown event rejection was not observed" >&2
  exit 1
fi

if ! grep -Eq "accepted[:=][[:space:]]*(true|True)" "${APPLY_EVENT_RESET_FILE}" ||
  ! grep -Eq "transitioned[:=][[:space:]]*(true|True)" "${APPLY_EVENT_RESET_FILE}" ||
  ! grep -q "current_state='RECOVERY'" "${APPLY_EVENT_RESET_FILE}" ||
  ! grep -q "runtime_error='NONE'" "${APPLY_EVENT_RESET_FILE}"; then
  echo "apply_event ResetFault did not enter RECOVERY" >&2
  exit 1
fi

if ! grep -Eq "accepted[:=][[:space:]]*(true|True)" "${APPLY_EVENT_RECOVERY_FILE}" ||
  ! grep -Eq "transitioned[:=][[:space:]]*(true|True)" "${APPLY_EVENT_RECOVERY_FILE}" ||
  ! grep -q "current_state='STANDBY'" "${APPLY_EVENT_RECOVERY_FILE}" ||
  ! grep -q "runtime_error='NONE'" "${APPLY_EVENT_RECOVERY_FILE}"; then
  echo "apply_event RecoveryDone did not return to STANDBY" >&2
  exit 1
fi

if ! grep -q "success=True" "${APPLY_EVENT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "state=STANDBY" "${APPLY_EVENT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_error=NONE" "${APPLY_EVENT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_severity=INFO" "${APPLY_EVENT_QUERY_OUTPUT_FILE}" ||
  ! grep -q "runtime_recoverable=1" "${APPLY_EVENT_QUERY_OUTPUT_FILE}"; then
  echo "query_status did not recover after apply_event chain" >&2
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
rm -f "${IMU_RELIABLE_MISMATCH_OUTPUT_FILE}"
rm -f "${JOINT_OUTPUT_FILE}"
rm -f "${HEARTBEAT_OUTPUT_FILE}"
rm -f "${IMU_HZ_OUTPUT_FILE}"
rm -f "${JOINT_HZ_OUTPUT_FILE}"
rm -f "${WATCHDOG_OUTPUT_FILE}"
rm -f "${SERVICE_OUTPUT_FILE}"
rm -f "${QUERY_SERVICE_LIST_FILE}"
rm -f "${QUERY_SERVICE_TYPE_FILE}"
rm -f "${QUERY_SERVICE_OUTPUT_FILE}"
rm -f "${APPLY_EVENT_TYPE_FILE}"
rm -f "${APPLY_EVENT_TIMEOUT_FILE}"
rm -f "${APPLY_EVENT_FAULT_QUERY_OUTPUT_FILE}"
rm -f "${APPLY_EVENT_UNKNOWN_FILE}"
rm -f "${APPLY_EVENT_RESET_FILE}"
rm -f "${APPLY_EVENT_RECOVERY_FILE}"
rm -f "${APPLY_EVENT_QUERY_OUTPUT_FILE}"
rm -f "${ACTION_LIST_FILE}"
rm -f "${ACTION_INTERFACE_FILE}"
rm -f "${ACTION_OUTPUT_FILE}"
rm -f "${ACTION_QUERY_OUTPUT_FILE}"
rm -f "${ACTION_REJECT_OUTPUT_FILE}"
rm -f "${ACTION_CANCEL_OUTPUT_FILE}"
rm -f "${STATE_MACHINE_OUTPUT_FILE}"
rm -f "${STATE_MACHINE_REVIEW_OUTPUT_FILE}"
rm -f "${INTEGRATION_REVIEW_OUTPUT_FILE}"
rm -f "${SENSOR_QOS_REVIEW_OUTPUT_FILE}"
rm -f "${COMMAND_RELIABILITY_REVIEW_OUTPUT_FILE}"
rm -f "${QUEUE_DEPTH_REVIEW_OUTPUT_FILE}"
echo "[ok] ROS2 runtime demo verified: state machine demo, phase 1 integration review, week 3 review, sensor best-effort QoS review, command reliability review, queue depth review, DDS QoS profiles, structured runtime_log fields, heartbeat pub/sub, watchdog timeout check, performance metrics, typed topics, runtime health, fault metadata, apply_event/query/reset services, and execute_task Action completion/rejection/cancellation are working"
