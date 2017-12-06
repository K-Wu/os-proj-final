cmake_minimum_required(VERSION 3.8)
project(os_proj2_c11)
find_package(Threads REQUIRED)
set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c)
add_executable(os_proj2_c11 ${SOURCE_FILES})
TARGET_LINK_LIBRARIES( os_proj2_c11 pthread)