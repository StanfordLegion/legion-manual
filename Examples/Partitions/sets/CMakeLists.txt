add_executable(sets sets.cc)
target_link_libraries(sets Legion::Legion)
add_test(NAME sets COMMAND $<TARGET_FILE:sets>)
