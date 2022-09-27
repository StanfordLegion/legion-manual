add_executable(pbf pbf.cc)
target_link_libraries(pbf Legion::Legion)
add_test(NAME pbf COMMAND $<TARGET_FILE:pbf>)
