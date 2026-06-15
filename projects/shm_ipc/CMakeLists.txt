cmake_minimum_required(VERSION 3.16)
project(shm_ipc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(writer src/writer.cpp)
add_executable(reader src/reader.cpp)

foreach(target writer reader)
    target_include_directories(${target} PRIVATE src)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    # Linux needs -lrt for shm_open/sem_open; macOS has them in libc
    if(UNIX AND NOT APPLE)
        target_link_libraries(${target} PRIVATE rt)
    endif()
endforeach()
