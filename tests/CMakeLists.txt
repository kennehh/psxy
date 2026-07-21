add_executable(test_bus test_bus.c)
target_link_libraries(test_bus PRIVATE psxy_core)
target_include_directories(test_bus PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(test_cpu test_cpu.c)
target_link_libraries(test_cpu PRIVATE psxy_core)
target_include_directories(test_cpu PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(test_single_step
    ${PROJECT_SOURCE_DIR}/src/core/psx.c
    ${PROJECT_SOURCE_DIR}/src/memory/bus.c
    ${PROJECT_SOURCE_DIR}/src/cpu/cpu.c
    ${PROJECT_SOURCE_DIR}/src/cpu/cop0.c
    ${PROJECT_SOURCE_DIR}/src/support/tty.c
    ${PROJECT_SOURCE_DIR}/src/support/loader.c
    ${PROJECT_SOURCE_DIR}/src/devices/gpu/gpu.c
    ${PROJECT_SOURCE_DIR}/src/devices/gpu/gp0.c
    ${PROJECT_SOURCE_DIR}/src/devices/gpu/gp1.c
    ${PROJECT_SOURCE_DIR}/src/devices/irq.c
    ${PROJECT_SOURCE_DIR}/src/devices/timers.c
    ${PROJECT_SOURCE_DIR}/src/memory/memctrl.c
    ${PROJECT_SOURCE_DIR}/src/devices/spu.c
    ${PROJECT_SOURCE_DIR}/src/devices/exp2.c
    test_single_step.c
)
target_compile_definitions(test_single_step PRIVATE PSXY_SINGLE_STEP_TEST_MODE)
target_link_libraries(test_single_step PRIVATE json-c)
target_include_directories(test_single_step PRIVATE ${PROJECT_SOURCE_DIR}/src .)


add_test(NAME bus.ram COMMAND test_bus ram)
add_test(NAME bus.exp1 COMMAND test_bus exp1)
add_test(NAME bus.scratchpad COMMAND test_bus scratchpad)
add_test(NAME bus.bios COMMAND test_bus bios)
add_test(NAME cpu.basic COMMAND test_cpu)

file(GLOB SINGLE_STEP_TESTS "fixtures/r3000a/v1/*.jsonl")
foreach(test_file ${SINGLE_STEP_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME cpu.single_step.${test_name} COMMAND test_single_step ${test_file})
endforeach()
