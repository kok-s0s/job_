cmake_minimum_required(VERSION 3.16)
project(socketcan_vcan_demo LANGUAGES CXX)

add_executable(can_frame_basics src/can_frame_basics.cpp)
target_compile_features(can_frame_basics PRIVATE cxx_std_17)

add_executable(vcan_loopback_demo src/vcan_loopback_demo.cpp)
target_compile_features(vcan_loopback_demo PRIVATE cxx_std_17)

add_executable(actuator_status_receiver src/actuator_status_receiver.cpp)
target_compile_features(actuator_status_receiver PRIVATE cxx_std_17)

add_executable(periodic_command_sender src/periodic_command_sender.cpp)
target_compile_features(periodic_command_sender PRIVATE cxx_std_17)

add_executable(can_runtime_fault_bridge src/can_runtime_fault_bridge.cpp)
target_compile_features(can_runtime_fault_bridge PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(can_frame_basics PRIVATE /W4)
  target_compile_options(vcan_loopback_demo PRIVATE /W4)
  target_compile_options(actuator_status_receiver PRIVATE /W4)
  target_compile_options(periodic_command_sender PRIVATE /W4)
  target_compile_options(can_runtime_fault_bridge PRIVATE /W4)
else()
  target_compile_options(can_frame_basics PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(vcan_loopback_demo PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(actuator_status_receiver PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(periodic_command_sender PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(can_runtime_fault_bridge PRIVATE -Wall -Wextra -Wpedantic)
endif()
