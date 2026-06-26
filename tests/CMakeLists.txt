add_executable(test_bus test_bus.c)
add_executable(test_cpu test_cpu.c)

target_link_libraries(test_bus PRIVATE bus_lib)
target_link_libraries(test_cpu PRIVATE cpu_lib bus_lib)

add_test(NAME test_bus COMMAND test_bus)
add_test(NAME test_cpu COMMAND test_cpu)
