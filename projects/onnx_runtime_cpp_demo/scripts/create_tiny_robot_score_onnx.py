#!/usr/bin/env python3
"""Create a tiny ONNX model without external Python packages.

The graph computes:
    anomaly_score = MatMul(robot_features, score_weight) + score_bias

Input shape:  [1, 3]
Output shape: [1, 1]
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


def varint(value: int) -> bytes:
    out = bytearray()
    while value >= 0x80:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value)
    return bytes(out)


def key(field_number: int, wire_type: int) -> bytes:
    return varint((field_number << 3) | wire_type)


def field_varint(field_number: int, value: int) -> bytes:
    return key(field_number, 0) + varint(value)


def field_bytes(field_number: int, value: bytes) -> bytes:
    return key(field_number, 2) + varint(len(value)) + value


def field_string(field_number: int, value: str) -> bytes:
    return field_bytes(field_number, value.encode("utf-8"))


def tensor_shape(dims: list[int]) -> bytes:
    message = bytearray()
    for dim in dims:
        message += field_bytes(1, field_varint(1, dim))
    return bytes(message)


def tensor_type(elem_type: int, dims: list[int]) -> bytes:
    message = bytearray()
    message += field_varint(1, elem_type)
    message += field_bytes(2, tensor_shape(dims))
    return bytes(message)


def value_info(name: str, dims: list[int], elem_type: int = 1) -> bytes:
    message = bytearray()
    message += field_string(1, name)
    message += field_bytes(2, field_bytes(1, tensor_type(elem_type, dims)))
    return bytes(message)


def tensor(name: str, dims: list[int], values: list[float], data_type: int = 1) -> bytes:
    raw = struct.pack("<" + "f" * len(values), *values)
    message = bytearray()
    for dim in dims:
        message += field_varint(1, dim)
    message += field_varint(2, data_type)
    message += field_string(8, name)
    message += field_bytes(9, raw)
    return bytes(message)


def node(op_type: str, inputs: list[str], outputs: list[str], name: str) -> bytes:
    message = bytearray()
    for item in inputs:
        message += field_string(1, item)
    for item in outputs:
        message += field_string(2, item)
    message += field_string(3, name)
    message += field_string(4, op_type)
    return bytes(message)


def opset(version: int) -> bytes:
    return field_varint(2, version)


def model() -> bytes:
    graph = bytearray()
    graph += field_bytes(1, node("MatMul", ["robot_features", "score_weight"], ["weighted_sum"], "score_matmul"))
    graph += field_bytes(1, node("Add", ["weighted_sum", "score_bias"], ["anomaly_score"], "score_bias_add"))
    graph += field_string(2, "tiny_robot_score")
    graph += field_bytes(5, tensor("score_weight", [3, 1], [0.2, -0.1, 0.7]))
    graph += field_bytes(5, tensor("score_bias", [1], [0.05]))
    graph += field_bytes(11, value_info("robot_features", [1, 3]))
    graph += field_bytes(12, value_info("anomaly_score", [1, 1]))

    message = bytearray()
    message += field_varint(1, 8)
    message += field_string(2, "job_week7_tiny_model")
    message += field_bytes(7, bytes(graph))
    message += field_bytes(8, opset(13))
    return bytes(message)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default="models/tiny_robot_score.onnx",
        help="Output ONNX file path relative to the project root.",
    )
    args = parser.parse_args()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(model())
    print(f"[ok] wrote {output} bytes={output.stat().st_size}")
    print("[model] input=robot_features shape=[1,3] output=anomaly_score shape=[1,1] opset=13")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
