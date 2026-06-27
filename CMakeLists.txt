cmake_minimum_required(VERSION 3.10)

project(psxy C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(psxy_core
    src/bus.c
    src/cpu.c
    src/exceptions.c
    src/tty.c
    src/loader.c
)

target_include_directories(psxy_core PUBLIC src)

target_compile_options(psxy_core PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wstrict-prototypes
    -Wmissing-prototypes
)

add_executable(psxy src/main.c)
target_link_libraries(psxy PRIVATE psxy_core)

enable_testing()
add_subdirectory(tests)
