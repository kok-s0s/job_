cmake_minimum_required(VERSION 3.16)
project(robocon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(robocon src/main.cpp)
target_include_directories(robocon PRIVATE src)
target_compile_options(robocon PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(robocon PRIVATE Threads::Threads)
