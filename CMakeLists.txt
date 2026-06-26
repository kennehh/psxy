cmake_minimum_required(VERSION 3.10)
project(psxy)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(bus_lib src/bus.c)
target_include_directories(bus_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()

add_subdirectory(tests)