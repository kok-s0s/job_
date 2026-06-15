cmake_minimum_required(VERSION 3.16)
project(state_machine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(robot_arm src/main.cpp)
target_include_directories(robot_arm PRIVATE src)
target_compile_options(robot_arm PRIVATE -Wall -Wextra -Wpedantic)
