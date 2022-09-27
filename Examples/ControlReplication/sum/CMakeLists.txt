add_executable(cp cp.cc)
target_link_libraries(cp Legion::Legion)
add_test(NAME cp COMMAND $<TARGET_FILE:cp>)
