#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

cd "${PROJECT_DIR}"

echo "[generate] tiny ONNX model"
python3 scripts/create_tiny_robot_score_onnx.py --output models/tiny_robot_score.onnx

echo "[build] onnx_model_inspector"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo "[run] inspect model metadata"
OUTPUT_FILE=$(mktemp)
./build/onnx_model_inspector models/tiny_robot_score.onnx | tee "${OUTPUT_FILE}"

grep -q "\[model\] graph=tiny_robot_score" "${OUTPUT_FILE}"
grep -q "\[input\] name=robot_features elem_type=float shape=\[1,3\]" "${OUTPUT_FILE}"
grep -q "\[output\] name=anomaly_score elem_type=float shape=\[1,1\]" "${OUTPUT_FILE}"
grep -q "\[node\] name=score_matmul op_type=MatMul" "${OUTPUT_FILE}"
grep -q "\[node\] name=score_bias_add op_type=Add" "${OUTPUT_FILE}"
grep -q "\[ok\] onnx model metadata loaded" "${OUTPUT_FILE}"

rm -f "${OUTPUT_FILE}"
echo "[ok] ONNX Runtime C++ demo verified: model generated, C++ loader built, input/output shapes inspected"
