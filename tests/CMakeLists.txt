add_library(psxy_test
    ../src/psx.c
    ../src/bus.c
    ../src/cpu.c
    ../src/exceptions.c
    ../src/tty.c
    ../src/bcache.c
)
target_include_directories(psxy_test PUBLIC ../src)

add_executable(test_bus test_bus.c)
target_link_libraries(test_bus PRIVATE psxy_test)

add_executable(test_cpu test_cpu.c)
target_link_libraries(test_cpu PRIVATE psxy_test)

add_executable(test_single_step test_single_step.c ../src/cpu.c ../src/exceptions.c ../src/bcache.c)
target_link_libraries(test_single_step PRIVATE json-c)
target_include_directories(test_single_step PRIVATE ../src .)
target_compile_definitions(test_single_step PRIVATE SINGLE_STEP_TEST_MODE)

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
