add_executable(equal equal.cc)
target_link_libraries(equal Legion::Legion)
add_test(NAME equal COMMAND $<TARGET_FILE:equal>)
