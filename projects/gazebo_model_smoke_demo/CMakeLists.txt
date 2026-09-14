cmake_minimum_required(VERSION 3.16)
project(gazebo_model_smoke_demo LANGUAGES CXX)

add_executable(gazebo_world_check src/gazebo_world_check.cpp)
target_compile_features(gazebo_world_check PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(gazebo_world_check PRIVATE /W4)
else()
  target_compile_options(gazebo_world_check PRIVATE -Wall -Wextra -Wpedantic)
endif()
