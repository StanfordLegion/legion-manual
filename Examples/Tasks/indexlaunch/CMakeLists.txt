add_executable(indexlaunch indexlaunch.cc)
target_link_libraries(indexlaunch Legion::Legion)
add_test(NAME indexlaunch COMMAND $<TARGET_FILE:indexlaunch>)
