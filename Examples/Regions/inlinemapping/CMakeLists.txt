add_executable(inlinemapping inlinemapping.cc)
target_link_libraries(inlinemapping Legion::Legion)
add_test(NAME inlinemapping COMMAND $<TARGET_FILE:inlinemapping>)
