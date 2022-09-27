add_executable(region_atomic atomic.cc)
target_link_libraries(region_atomic Legion::Legion)
add_test(NAME region_atomic COMMAND $<TARGET_FILE:region_atomic>)
