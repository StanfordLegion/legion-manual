add_executable(pbr pbr.cc)
target_link_libraries(pbr Legion::Legion)
add_test(NAME pbr COMMAND $<TARGET_FILE:pbr>)
