# ONNX Runtime C++ Demo

对应练习：
- [2026-08-24：准备一个小 ONNX 模型](/roadmap/daily/2026-08-24)
- [2026-08-25：C++ 加载 ONNX 模型元信息](/roadmap/daily/2026-08-25)
- [2026-08-26：封装固定输入推理函数](/roadmap/daily/2026-08-26)
- [2026-08-27：接入 ROS2 推理节点](/roadmap/daily/2026-08-27)

这是第 7 周“ONNX Runtime C++ 推理部署”的起步项目。当前先完成两个可验证目标：生成一个小 ONNX 模型，并用 C++ 加载模型文件、解析输入输出 shape 和图结构。

源码目录：

```txt
projects/onnx_runtime_cpp_demo
```

## 当前能力

- `scripts/create_tiny_robot_score_onnx.py` 不依赖外部 Python 包，直接生成一个最小 ONNX protobuf 模型。
- `models/tiny_robot_score.onnx` 表达 `robot_features[1,3] -> anomaly_score[1,1]` 的小推理图。
- `src/onnx_model_inspector.cpp` 用 C++17 读取 ONNX 文件并解析 ModelProto / GraphProto / ValueInfo / Node / Tensor 的关键字段。
- `src/tiny_robot_inference.hpp` 封装固定输入推理函数，输出稳定 score 和状态。
- `src/onnx_inference_demo.cpp` 验证固定输入 `[0.500,-1.000,0.200]` 的结果为 `score=0.390 status=OK`。
- `scripts/verify_onnx_runtime_cpp_demo.sh` 负责生成模型、构建 C++ 程序、运行检查并 grep 关键验收输出。

## 验收命令

```bash
cd projects/onnx_runtime_cpp_demo
bash scripts/verify_onnx_runtime_cpp_demo.sh
```

关键输出：

```txt
[model] graph=tiny_robot_score inputs=1 outputs=1 nodes=2 initializers=2
[input] name=robot_features elem_type=float shape=[1,3]
[output] name=anomaly_score elem_type=float shape=[1,1]
[node] name=score_matmul op_type=MatMul
[node] name=score_bias_add op_type=Add
[ok] onnx model metadata loaded
[inference] model=tiny_robot_score input=[0.500,-1.000,0.200] score=0.390 status=OK
[ok] fixed input inference stable
```

## 面试表达

AI 模型工程化的第一步不是马上写复杂推理逻辑，而是把模型契约固定下来：模型文件是否存在、opset 是否符合预期、输入输出名字和 shape 是否能被程序读取、错误信息是否清楚。这个项目先把 C++ 侧加载边界跑通，后面再替换为真实 ONNX Runtime Session 和 ROS2 推理节点。
