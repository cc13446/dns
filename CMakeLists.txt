cmake_minimum_required(VERSION 3.22)
project(dns C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/target)

aux_source_directory(. DIR_SRCS)

add_executable(dns ${DIR_SRCS})