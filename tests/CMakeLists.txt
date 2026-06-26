add_executable(test_bus test_bus.c)

target_link_libraries(test_bus PRIVATE bus_lib)

add_test(NAME test_bus COMMAND test_bus)
