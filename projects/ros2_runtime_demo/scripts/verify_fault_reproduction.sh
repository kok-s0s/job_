#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
PACKAGE_NAME=robot_runtime_demo
LAUNCH_FILE=runtime_demo.launch.py
SESSION_ID=${ROBOT_RUNTIME_SESSION_ID:-fault_repro_2026_08_20}
export ROBOT_RUNTIME_SESSION_ID="${SESSION_ID}"

wait_for_service() {
  local service_name=$1
  local attempts=${2:-30}
  for ((i = 0; i < attempts; ++i)); do
    if ros2 service list 2>/dev/null | grep -qx "${service_name}"; then
      return 0
    fi
    sleep 0.5
  done
  echo "service not discovered: ${service_name}" >&2
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

echo "[review] runtime_fault_reproduction_review"
FAULT_REVIEW_OUTPUT_FILE=$(mktemp)
ros2 run "${PACKAGE_NAME}" runtime_fault_reproduction_review >"${FAULT_REVIEW_OUTPUT_FILE}" 2>&1

LAUNCH_OUTPUT_FILE=$(mktemp)
BASELINE_QUERY_FILE=$(mktemp)
FAULT_EVENT_FILE=$(mktemp)
FAULT_QUERY_FILE=$(mktemp)
RESET_EVENT_FILE=$(mktemp)
RECOVERY_EVENT_FILE=$(mktemp)
RECOVERY_QUERY_FILE=$(mktemp)

ros2 launch "${PACKAGE_NAME}" "${LAUNCH_FILE}" >"${LAUNCH_OUTPUT_FILE}" 2>&1 &
LAUNCH_PID=$!

cleanup() {
  if kill -0 "${LAUNCH_PID}" >/dev/null 2>&1; then
    kill "${LAUNCH_PID}" >/dev/null 2>&1 || true
    wait "${LAUNCH_PID}" >/dev/null 2>&1 || true
  fi
  rm -f "${FAULT_REVIEW_OUTPUT_FILE}" "${LAUNCH_OUTPUT_FILE}" "${BASELINE_QUERY_FILE}" \
    "${FAULT_EVENT_FILE}" "${FAULT_QUERY_FILE}" "${RESET_EVENT_FILE}" \
    "${RECOVERY_EVENT_FILE}" "${RECOVERY_QUERY_FILE}"
}
trap cleanup EXIT

wait_for_service /runtime/query_status
wait_for_service /runtime/apply_event

sleep 3

echo "[fault] baseline query"
timeout 10s ros2 service call /runtime/query_status std_srvs/srv/Trigger "{}" >"${BASELINE_QUERY_FILE}" 2>&1

echo "[fault] inject SensorTimeout"
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: SensorTimeout}" >"${FAULT_EVENT_FILE}" 2>&1

echo "[fault] verify FAULT"
timeout 10s ros2 service call /runtime/query_status std_srvs/srv/Trigger "{}" >"${FAULT_QUERY_FILE}" 2>&1

echo "[fault] recover"
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: ResetFault}" >"${RESET_EVENT_FILE}" 2>&1
timeout 10s ros2 service call \
  /runtime/apply_event \
  robot_runtime_demo/srv/ApplyRuntimeEvent \
  "{event: RecoveryDone}" >"${RECOVERY_EVENT_FILE}" 2>&1
timeout 10s ros2 service call /runtime/query_status std_srvs/srv/Trigger "{}" >"${RECOVERY_QUERY_FILE}" 2>&1

sleep 1

cat "${FAULT_REVIEW_OUTPUT_FILE}"
cat "${BASELINE_QUERY_FILE}"
cat "${FAULT_EVENT_FILE}"
cat "${FAULT_QUERY_FILE}"
cat "${RESET_EVENT_FILE}"
cat "${RECOVERY_EVENT_FILE}"
cat "${RECOVERY_QUERY_FILE}"
grep -E "\[session\]|\[runtime_log\]|\[heartbeat\]|\[perf\]" "${LAUNCH_OUTPUT_FILE}" | tail -60 || true

if ! grep -q "\[ok\] fault reproduction review ready" "${FAULT_REVIEW_OUTPUT_FILE}"; then
  echo "fault reproduction review output was not observed" >&2
  exit 1
fi

if ! grep -q "session_id=${SESSION_ID}" "${BASELINE_QUERY_FILE}" ||
  ! grep -q "state=STANDBY" "${BASELINE_QUERY_FILE}" ||
  ! grep -q "runtime_error=NONE" "${BASELINE_QUERY_FILE}"; then
  echo "baseline runtime query did not include expected session and healthy state" >&2
  exit 1
fi

if ! grep -Eq "accepted[:=][[:space:]]*(true|True)" "${FAULT_EVENT_FILE}" ||
  ! grep -Eq "transitioned[:=][[:space:]]*(true|True)" "${FAULT_EVENT_FILE}" ||
  ! grep -q "current_state='FAULT'" "${FAULT_EVENT_FILE}" ||
  ! grep -q "runtime_error='SENSOR_TIMEOUT'" "${FAULT_EVENT_FILE}"; then
  echo "SensorTimeout did not transition runtime into FAULT" >&2
  exit 1
fi

if ! grep -q "success=False" "${FAULT_QUERY_FILE}" ||
  ! grep -q "session_id=${SESSION_ID}" "${FAULT_QUERY_FILE}" ||
  ! grep -q "state=FAULT" "${FAULT_QUERY_FILE}" ||
  ! grep -q "runtime_error=SENSOR_TIMEOUT" "${FAULT_QUERY_FILE}" ||
  ! grep -q "runtime_severity=CRITICAL" "${FAULT_QUERY_FILE}"; then
  echo "fault query did not include expected fault metadata" >&2
  exit 1
fi

if ! grep -q "current_state='RECOVERY'" "${RESET_EVENT_FILE}" ||
  ! grep -q "current_state='STANDBY'" "${RECOVERY_EVENT_FILE}" ||
  ! grep -q "success=True" "${RECOVERY_QUERY_FILE}" ||
  ! grep -q "session_id=${SESSION_ID}" "${RECOVERY_QUERY_FILE}" ||
  ! grep -q "state=STANDBY" "${RECOVERY_QUERY_FILE}" ||
  ! grep -q "runtime_error=NONE" "${RECOVERY_QUERY_FILE}"; then
  echo "recovery chain did not return runtime to STANDBY" >&2
  exit 1
fi

if ! grep -q "session_id=${SESSION_ID}" "${LAUNCH_OUTPUT_FILE}" ||
  ! grep -q "\[runtime_log\].*session_id=${SESSION_ID}.*event=state_transition" "${LAUNCH_OUTPUT_FILE}" ||
  ! grep -q "\[heartbeat\].*session_id=${SESSION_ID}" "${LAUNCH_OUTPUT_FILE}" ||
  ! grep -q "\[perf\].*session_id=${SESSION_ID}" "${LAUNCH_OUTPUT_FILE}"; then
  echo "session_id was not observed across runtime logs, heartbeat, and perf output" >&2
  exit 1
fi

echo "[ok] deterministic fault reproduction verified: session_id=${SESSION_ID}, SensorTimeout -> FAULT, ResetFault -> RecoveryDone -> STANDBY"
