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

rm -f "${OUTPUT_FILE}"
echo "[ok] SocketCAN vcan demo verified: CAN frame structure and basic commands are documented"
