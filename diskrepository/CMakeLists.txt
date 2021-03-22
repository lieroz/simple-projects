cmake_minimum_required(VERSION 3.6)
project(diskrepository)
set(TARGET ${PROJECT_NAME})

add_definitions("-Wall -Wextra -Werror -Wno-unused-parameter -std=c++17 -fPIE -fomit-frame-pointer")

enable_testing()
add_subdirectory(tests)
