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

echo "[run] fixed input inference"
INFERENCE_OUTPUT_FILE=$(mktemp)
./build/onnx_inference_demo | tee "${INFERENCE_OUTPUT_FILE}"
grep -q "\[inference\] model=tiny_robot_score input=\[0.500,-1.000,0.200\] score=0.390 status=OK" "${INFERENCE_OUTPUT_FILE}"
grep -q "\[ok\] fixed input inference stable" "${INFERENCE_OUTPUT_FILE}"

echo "[run] inference timing stats"
STATS_OUTPUT_FILE=$(mktemp)
./build/onnx_inference_stats_demo | tee "${STATS_OUTPUT_FILE}"
grep -q "\[inference_stats\] count=3 avg_ms=.* max_ms=.* failures=1" "${STATS_OUTPUT_FILE}"
grep -q "\[ok\] inference timing stats ready" "${STATS_OUTPUT_FILE}"

rm -f "${OUTPUT_FILE}"
rm -f "${INFERENCE_OUTPUT_FILE}"
rm -f "${STATS_OUTPUT_FILE}"
echo "[ok] ONNX Runtime C++ demo verified: model generated, C++ loader built, input/output shapes inspected, fixed input inference stable, timing stats ready"
