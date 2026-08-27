cmake_minimum_required(VERSION 3.8)
project(robot_runtime_demo LANGUAGES C CXX)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(action_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "action/ExecuteTask.action"
  "srv/ApplyRuntimeEvent.srv"
)

add_executable(runtime_node src/runtime_node.cpp)
ament_target_dependencies(runtime_node action_msgs rclcpp rclcpp_action sensor_msgs std_msgs std_srvs)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(runtime_node "${cpp_typesupport_target}")

add_executable(sensor_sim_node src/sensor_sim_node.cpp)
ament_target_dependencies(sensor_sim_node rclcpp sensor_msgs std_msgs)

add_executable(inference_node src/inference_node.cpp)
ament_target_dependencies(inference_node rclcpp sensor_msgs std_msgs)

add_executable(heartbeat_monitor_node src/heartbeat_monitor_node.cpp)
ament_target_dependencies(heartbeat_monitor_node rclcpp std_msgs)

add_executable(watchdog_node src/watchdog_node.cpp)
ament_target_dependencies(watchdog_node rclcpp std_msgs)
target_link_libraries(watchdog_node "${cpp_typesupport_target}")

add_executable(action_cancel_test_client src/action_cancel_test_client.cpp)
ament_target_dependencies(action_cancel_test_client action_msgs rclcpp rclcpp_action)
target_link_libraries(action_cancel_test_client "${cpp_typesupport_target}")

add_executable(runtime_state_machine_demo src/runtime_state_machine_demo.cpp)
add_executable(runtime_state_machine_review src/runtime_state_machine_review.cpp)
add_executable(runtime_integration_review src/runtime_integration_review.cpp)
add_executable(runtime_sensor_qos_review src/runtime_sensor_qos_review.cpp)
add_executable(runtime_command_reliability_review src/runtime_command_reliability_review.cpp)
add_executable(runtime_queue_depth_review src/runtime_queue_depth_review.cpp)
add_executable(runtime_qos_interview_review src/runtime_qos_interview_review.cpp)
add_executable(runtime_rosbag_recording_review src/runtime_rosbag_recording_review.cpp)
add_executable(runtime_session_trace_review src/runtime_session_trace_review.cpp)
add_executable(runtime_fault_reproduction_review src/runtime_fault_reproduction_review.cpp)
add_executable(runtime_data_loop_review src/runtime_data_loop_review.cpp)

target_compile_features(runtime_node PRIVATE cxx_std_17)
target_compile_features(sensor_sim_node PRIVATE cxx_std_17)
target_compile_features(inference_node PRIVATE cxx_std_17)
target_compile_features(heartbeat_monitor_node PRIVATE cxx_std_17)
target_compile_features(watchdog_node PRIVATE cxx_std_17)
target_compile_features(action_cancel_test_client PRIVATE cxx_std_17)
target_compile_features(runtime_state_machine_demo PRIVATE cxx_std_17)
target_compile_features(runtime_state_machine_review PRIVATE cxx_std_17)
target_compile_features(runtime_integration_review PRIVATE cxx_std_17)
target_compile_features(runtime_sensor_qos_review PRIVATE cxx_std_17)
target_compile_features(runtime_command_reliability_review PRIVATE cxx_std_17)
target_compile_features(runtime_queue_depth_review PRIVATE cxx_std_17)
target_compile_features(runtime_qos_interview_review PRIVATE cxx_std_17)
target_compile_features(runtime_rosbag_recording_review PRIVATE cxx_std_17)
target_compile_features(runtime_session_trace_review PRIVATE cxx_std_17)
target_compile_features(runtime_fault_reproduction_review PRIVATE cxx_std_17)
target_compile_features(runtime_data_loop_review PRIVATE cxx_std_17)

install(TARGETS
  runtime_node
  sensor_sim_node
  inference_node
  heartbeat_monitor_node
  watchdog_node
  action_cancel_test_client
  runtime_state_machine_demo
  runtime_state_machine_review
  runtime_integration_review
  runtime_sensor_qos_review
  runtime_command_reliability_review
  runtime_queue_depth_review
  runtime_qos_interview_review
  runtime_rosbag_recording_review
  runtime_session_trace_review
  runtime_fault_reproduction_review
  runtime_data_loop_review
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  action
  launch
  srv
  DESTINATION share/${PROJECT_NAME}
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
