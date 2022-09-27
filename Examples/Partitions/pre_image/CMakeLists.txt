add_executable(preimage preimage.cc)
target_link_libraries(preimage Legion::Legion)
add_test(NAME preimage COMMAND $<TARGET_FILE:preimage>)
