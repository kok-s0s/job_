cmake_minimum_required(VERSION 3.16)
project(urdf_two_joint_arm_demo LANGUAGES CXX)

add_executable(urdf_structure_check src/urdf_structure_check.cpp)
target_compile_features(urdf_structure_check PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(urdf_structure_check PRIVATE /W4)
else()
  target_compile_options(urdf_structure_check PRIVATE -Wall -Wextra -Wpedantic)
endif()
