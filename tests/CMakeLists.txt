add_executable(test_bus test_bus.c)
target_link_libraries(test_bus PRIVATE psxy_core)
target_include_directories(test_bus PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(test_cpu test_cpu.c)
target_link_libraries(test_cpu PRIVATE psxy_core)
target_include_directories(test_cpu PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(test_single_step
    ${PROJECT_SOURCE_DIR}/src/psx.c
    ${PROJECT_SOURCE_DIR}/src/bus.c
    ${PROJECT_SOURCE_DIR}/src/cpu.c
    ${PROJECT_SOURCE_DIR}/src/cop0.c
    ${PROJECT_SOURCE_DIR}/src/tty.c
    ${PROJECT_SOURCE_DIR}/src/loader.c
    ${PROJECT_SOURCE_DIR}/src/gpu.c
    ${PROJECT_SOURCE_DIR}/src/irq.c
    ${PROJECT_SOURCE_DIR}/src/timers.c
    ${PROJECT_SOURCE_DIR}/src/memctrl.c
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
