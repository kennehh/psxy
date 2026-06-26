cmake_minimum_required(VERSION 3.10)
project(psxy)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(bus_lib src/bus.c)
add_library(cpu_lib src/cpu.c src/exc.c)
target_include_directories(bus_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(cpu_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()

add_subdirectory(tests)
