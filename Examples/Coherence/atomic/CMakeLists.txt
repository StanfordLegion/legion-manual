add_executable(atomic atomic.cc)
target_link_libraries(atomic Legion::Legion)
add_test(NAME atomic COMMAND $<TARGET_FILE:atomic>)
