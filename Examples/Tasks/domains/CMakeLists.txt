add_executable(domains domains.cc)
target_link_libraries(domains Legion::Legion)
add_test(NAME domains COMMAND $<TARGET_FILE:domains>)
