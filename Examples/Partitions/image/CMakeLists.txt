add_executable(image image.cc)
target_link_libraries(image Legion::Legion)
add_test(NAME image COMMAND $<TARGET_FILE:image>)
