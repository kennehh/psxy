cmake_minimum_required(VERSION 3.23)

project(psxy C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PSXY_ENABLE_TRACE "Enable instruction/frame tracing" OFF)
option(PSXY_ENABLE_DEBUGGER "Enable built-in emulator debugger" ON)
option(PSXY_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)
option(PSXY_ENABLE_PROFILING "Enable profiling-friendly build flags" OFF)
option(PSXY_ENABLE_LTO "Enable link-time optimization" OFF)
option(PSXY_ENABLE_NATIVE_OPT "Enable native CPU-specific optimization" OFF)
option(PSXY_STRICT_WARNINGS "Treat warnings as errors" OFF)

add_library(psxy_core
    src/core/psx.c
    src/memory/bus.c
    src/cpu/cpu.c
    src/cpu/cop0.c
    src/support/tty.c
    src/support/loader.c
    src/devices/gpu/gpu.c
    src/devices/gpu/gp0.c
    src/devices/gpu/gp1.c
    src/devices/irq.c
    src/devices/timers.c
    src/memory/memctrl.c
    src/devices/spu.c
    src/devices/exp2.c
)

target_include_directories(psxy_core PRIVATE src)

target_compile_definitions(psxy_core
    PRIVATE
        $<$<BOOL:${PSXY_ENABLE_TRACE}>:PSXY_TRACE=1>
        $<$<BOOL:${PSXY_ENABLE_DEBUGGER}>:PSXY_DEBUGGER=1>
        $<$<BOOL:${PSXY_ENABLE_TESTS}>:PSXY_TESTS=1>
)

target_compile_options(psxy_core PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall
        -Wextra
        -Wshadow
        -Wstrict-prototypes
        -Wmissing-prototypes
        -Werror=implicit-function-declaration
        -Wswitch-enum
        -Wundef
        -Wno-initializer-overrides
        -Wno-override-init
        -Wno-gnu-label-as-value
        -Wno-unused-parameter
    >
    $<$<C_COMPILER_ID:MSVC>:
        /W4
    >
)

if(PSXY_ENABLE_DEBUGGER)
    target_compile_options(psxy_core PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
            -Og
            -g3
            -fno-omit-frame-pointer
        >
    )
endif()

if(PSXY_ENABLE_SANITIZERS)
    target_compile_options(psxy_core PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
            -fsanitize=address,undefined
            -O1
            -g3
            -fno-omit-frame-pointer
        >
    )

    target_link_options(psxy_core PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
            -fsanitize=address,undefined
        >
    )
endif()

if(PSXY_ENABLE_PROFILING)
    target_compile_options(psxy_core PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
            -O3
            -g
            -fno-omit-frame-pointer
        >
    )
endif()

if(PSXY_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported)
        set_property(TARGET psxy_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO/LTO not supported: ${ipo_error}")
    endif()
endif()

add_executable(psxy
    src/main.c
)
target_include_directories(psxy PRIVATE src)
target_link_libraries(psxy PRIVATE psxy_core)

enable_testing()
add_subdirectory(tests)
