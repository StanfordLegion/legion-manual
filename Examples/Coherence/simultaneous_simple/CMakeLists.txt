add_executable(sim_simple sim.cc)
target_link_libraries(sim_simple Legion::Legion)
add_test(NAME sim_simple COMMAND $<TARGET_FILE:sim_simple>)
