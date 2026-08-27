cmake_minimum_required(VERSION 3.16)
project(onnx_runtime_cpp_demo LANGUAGES CXX)

add_executable(onnx_model_inspector src/onnx_model_inspector.cpp)
target_compile_features(onnx_model_inspector PRIVATE cxx_std_17)

add_executable(onnx_inference_demo src/onnx_inference_demo.cpp)
target_compile_features(onnx_inference_demo PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(onnx_model_inspector PRIVATE /W4)
  target_compile_options(onnx_inference_demo PRIVATE /W4)
else()
  target_compile_options(onnx_model_inspector PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(onnx_inference_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
