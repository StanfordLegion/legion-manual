add_executable(futures futures.cc)
target_link_libraries(futures Legion::Legion)
add_test(NAME futures COMMAND $<TARGET_FILE:futures>)
