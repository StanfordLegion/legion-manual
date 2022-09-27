add_executable(physicalregions physicalregions.cc)
target_link_libraries(physicalregions Legion::Legion)
add_test(NAME physicalregions COMMAND $<TARGET_FILE:physicalregions>)
