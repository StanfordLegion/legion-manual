add_executable(sim sim.cc)
target_link_libraries(sim Legion::Legion)
add_test(NAME sim COMMAND $<TARGET_FILE:sim>)
