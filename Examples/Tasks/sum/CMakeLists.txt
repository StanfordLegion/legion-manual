add_executable(sum sum.cc)
target_link_libraries(sum Legion::Legion)
add_test(NAME sum COMMAND $<TARGET_FILE:sum>)
