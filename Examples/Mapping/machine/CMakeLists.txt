add_executable(machine machine.cc)
target_link_libraries(machine Legion::Legion)
add_test(NAME machine COMMAND $<TARGET_FILE:machine>)
