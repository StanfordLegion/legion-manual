add_executable(subtasks subtasks.cc)
target_link_libraries(subtasks Legion::Legion)
add_test(NAME subtasks COMMAND $<TARGET_FILE:subtasks>)
