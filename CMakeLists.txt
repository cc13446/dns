cmake_minimum_required(VERSION 3.22)
project(dns C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/target)

add_executable(dns main.c)
