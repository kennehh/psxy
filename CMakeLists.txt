cmake_minimum_required(VERSION 3.10)

project(psxy C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(psxy
    src/main.c
    src/psx.c
    src/bus.c
    src/cpu.c
    src/exceptions.c
    src/tty.c
    src/loader.c
    src/gpu.c
    src/irq.c
)

target_include_directories(psxy PRIVATE src)

target_compile_options(psxy PRIVATE
    -Wall
    # -Wextra
    # -Wshadow
    # -Wstrict-prototypes
    # -Wmissing-prototypes

    # -O3
    -g
    -fno-omit-frame-pointer
)

enable_testing()
add_subdirectory(tests)
