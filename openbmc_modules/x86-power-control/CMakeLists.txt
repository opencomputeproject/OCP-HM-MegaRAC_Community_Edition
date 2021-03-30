cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/i2c/inc)

add_subdirectory(i2c)
add_subdirectory(power-control-x86)
