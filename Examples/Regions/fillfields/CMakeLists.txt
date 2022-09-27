add_executable(fillfields fillfields.cc)
target_link_libraries(fillfields Legion::Legion)
add_test(NAME fillfields COMMAND $<TARGET_FILE:fillfields>)
