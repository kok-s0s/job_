cmake_minimum_required(VERSION 3.16)
project(urdf_two_joint_arm_demo LANGUAGES CXX)

add_executable(urdf_structure_check src/urdf_structure_check.cpp)
target_compile_features(urdf_structure_check PRIVATE cxx_std_17)

add_executable(tf_tree_review src/tf_tree_review.cpp)
target_compile_features(tf_tree_review PRIVATE cxx_std_17)

add_executable(sensor_frame_mount_check src/sensor_frame_mount_check.cpp)
target_compile_features(sensor_frame_mount_check PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(urdf_structure_check PRIVATE /W4)
  target_compile_options(tf_tree_review PRIVATE /W4)
  target_compile_options(sensor_frame_mount_check PRIVATE /W4)
else()
  target_compile_options(urdf_structure_check PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(tf_tree_review PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(sensor_frame_mount_check PRIVATE -Wall -Wextra -Wpedantic)
endif()
