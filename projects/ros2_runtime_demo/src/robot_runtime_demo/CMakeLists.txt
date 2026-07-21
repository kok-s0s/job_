cmake_minimum_required(VERSION 3.8)
project(robot_runtime_demo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

add_executable(runtime_node src/runtime_node.cpp)
ament_target_dependencies(runtime_node rclcpp std_msgs std_srvs)

add_executable(sensor_sim_node src/sensor_sim_node.cpp)
ament_target_dependencies(sensor_sim_node rclcpp std_msgs)

target_compile_features(runtime_node PRIVATE cxx_std_17)
target_compile_features(sensor_sim_node PRIVATE cxx_std_17)

install(TARGETS
  runtime_node
  sensor_sim_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
