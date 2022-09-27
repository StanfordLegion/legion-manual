add_executable(sumtree sumtree.cc)
target_link_libraries(sumtree Legion::Legion)
add_test(NAME sumtree COMMAND $<TARGET_FILE:sumtree>)
