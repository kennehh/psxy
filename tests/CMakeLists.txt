add_executable(test_bus test_bus.c)
target_link_libraries(test_bus PRIVATE bus_lib)

add_executable(test_cpu test_cpu.c)
target_link_libraries(test_cpu PRIVATE cpu_lib bus_lib)

add_executable(test_single_step test_single_step.c)
target_link_libraries(test_single_step PRIVATE cpu_lib bus_lib json-c)

add_test(NAME bus.ram COMMAND test_bus ram)
add_test(NAME bus.exp1 COMMAND test_bus exp1)
add_test(NAME bus.scratchpad COMMAND test_bus scratchpad)
add_test(NAME bus.bios COMMAND test_bus bios)
add_test(NAME cpu.basic COMMAND test_cpu)

file(GLOB SINGLE_STEP_TESTS "../tests/fixtures/r3000a/v1/*.jsonl")
foreach(test_file ${SINGLE_STEP_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME cpu.single_step.${test_name} COMMAND test_single_step ${test_file})
endforeach()
