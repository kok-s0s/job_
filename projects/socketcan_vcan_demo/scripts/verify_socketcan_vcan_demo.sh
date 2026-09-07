#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

cd "${PROJECT_DIR}"

echo "[build] can_frame_basics"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo "[run] can_frame_basics"
OUTPUT_FILE=$(mktemp)
./build/can_frame_basics | tee "${OUTPUT_FILE}"

grep -q "\[socketcan\] basic command checklist" "${OUTPUT_FILE}"
grep -q "sudo modprobe vcan" "${OUTPUT_FILE}"
grep -q "candump vcan0" "${OUTPUT_FILE}"
grep -q "\[can_frame\] raw=123#1122334455667788 id=0x123 dlc=8 data=\"11 22 33 44 55 66 77 88\"" "${OUTPUT_FILE}"
grep -q "\[can_frame\] raw=321#AABBCCDD id=0x321 dlc=4 data=\"AA BB CC DD\"" "${OUTPUT_FILE}"
grep -q "\[ok\] CAN frame basics verified" "${OUTPUT_FILE}"

echo "[run] vcan_loopback_demo"
./build/vcan_loopback_demo | tee -a "${OUTPUT_FILE}"

grep -q "\[vcan_loopback\] tx=\"cansend vcan0 123#01020304\"" "${OUTPUT_FILE}"
grep -q "\[vcan_loopback\] received id=0x123 dlc=4 data=\"01 02 03 04\"" "${OUTPUT_FILE}"
grep -q "\[ok\] vcan loopback command path verified" "${OUTPUT_FILE}"

echo "[run] actuator_status_receiver"
./build/actuator_status_receiver | tee -a "${OUTPUT_FILE}"

grep -q "\[actuator_status\] raw=181#E8032C0100000000 node_id=1 position_rad=1.000 velocity_rad_s=0.300 fault=0 fault_code=0" "${OUTPUT_FILE}"
grep -q "\[actuator_status\] raw=182#18FC38FF01070000 node_id=2 position_rad=-1.000 velocity_rad_s=-0.200 fault=1 fault_code=7" "${OUTPUT_FILE}"
grep -q "\[ok\] actuator status receiver decoded position velocity and fault" "${OUTPUT_FILE}"

echo "[run] periodic_command_sender"
./build/periodic_command_sender | tee -a "${OUTPUT_FILE}"

grep -q "\[command_sender\] target_hz=50 period_ms=20 sample_count=10" "${OUTPUT_FILE}"
grep -q "\[command_tx\] tick=9 t_ms=180 cansend=\"cansend vcan0 201#" "${OUTPUT_FILE}"
grep -q "\[ok\] periodic command sender generated 50Hz control frames" "${OUTPUT_FILE}"

echo "[run] can_runtime_fault_bridge"
./build/can_runtime_fault_bridge | tee -a "${OUTPUT_FILE}"

grep -q "\[can_watchdog\] now_ms=150 age_ms=150 timeout_ms=100 state=FAULT error=CAN_TIMEOUT" "${OUTPUT_FILE}"
grep -q "\[can_bridge\] now_ms=0 raw=182#18FC38FF01070000 state=FAULT error=CAN_ACTUATOR_FAULT" "${OUTPUT_FILE}"
grep -q "\[ok\] CAN timeout and actuator fault map to runtime Fault" "${OUTPUT_FILE}"

rm -f "${OUTPUT_FILE}"
echo "[ok] SocketCAN vcan demo verified: CAN basics, vcan loopback, actuator status, 50Hz commands, and runtime Fault mapping are working"
