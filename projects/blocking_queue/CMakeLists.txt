cmake_minimum_required(VERSION 3.16)
project(blocking_queue CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(demo src/main.cpp)
target_include_directories(demo PRIVATE src)
target_compile_options(demo PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(demo PRIVATE Threads::Threads)
