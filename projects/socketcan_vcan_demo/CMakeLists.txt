cmake_minimum_required(VERSION 3.16)
project(socketcan_vcan_demo LANGUAGES CXX)

add_executable(can_frame_basics src/can_frame_basics.cpp)
target_compile_features(can_frame_basics PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(can_frame_basics PRIVATE /W4)
else()
  target_compile_options(can_frame_basics PRIVATE -Wall -Wextra -Wpedantic)
endif()
