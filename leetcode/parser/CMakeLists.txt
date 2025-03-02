cmake_minimum_required(VERSION 3.18)

project(parser)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/build>)

set(PARSER_SOURCES parser.c)
add_executable(parser ${PARSER_SOURCES})
