add_executable(registration registration.cc)
target_link_libraries(registration Legion::Legion)
add_test(NAME registration COMMAND $<TARGET_FILE:registration>)
