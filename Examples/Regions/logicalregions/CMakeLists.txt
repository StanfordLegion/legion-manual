add_executable(logicalregions logicalregions.cc)
target_link_libraries(logicalregions Legion::Legion)
add_test(NAME logicalregions COMMAND $<TARGET_FILE:logicalregions>)
