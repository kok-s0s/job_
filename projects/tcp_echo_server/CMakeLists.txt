cmake_minimum_required(VERSION 3.16)
project(tcp_echo_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

foreach(target server client)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(${target} PRIVATE Threads::Threads)
endforeach()
