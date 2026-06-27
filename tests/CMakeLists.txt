add_executable(test_bus test_bus.c)
add_executable(test_cpu test_cpu.c)
add_executable(test_single_step test_single_step.c)

target_link_libraries(test_bus PRIVATE bus_lib)
target_link_libraries(test_cpu PRIVATE cpu_lib bus_lib)
target_link_libraries(test_single_step PRIVATE cpu_lib bus_lib json-c)

file(GLOB SINGLE_STEP_TESTS "../tests/fixtures/r3000a/v1/*.jsonl")

add_test(NAME test_bus_ram COMMAND test_bus ram)
add_test(NAME test_bus_exp1 COMMAND test_bus exp1)
add_test(NAME test_bus_scratchpad COMMAND test_bus scratchpad)
add_test(NAME test_bus_bios COMMAND test_bus bios)
add_test(NAME test_cpu COMMAND test_cpu)

foreach(test_file ${SINGLE_STEP_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME test_single_step_${test_name} COMMAND test_single_step ${test_file})
endforeach()
