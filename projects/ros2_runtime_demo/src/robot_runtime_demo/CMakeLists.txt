cmake_minimum_required(VERSION 3.8)
project(robot_runtime_demo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

add_executable(runtime_node src/runtime_node.cpp)
ament_target_dependencies(runtime_node rclcpp)

target_compile_features(runtime_node PRIVATE cxx_std_17)

install(TARGETS
  runtime_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
