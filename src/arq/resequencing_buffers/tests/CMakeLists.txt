# Stop and wait RS buffer MUT
add_executable(stop_and_wait_rs_test stop_and_wait_rs_test.cpp)
target_link_libraries(stop_and_wait_rs_test PRIVATE Catch2::Catch2WithMain rs_buffers util)
catch_discover_tests(stop_and_wait_rs_test)

# Dummy SCTP RS buffer MUT
add_executable(dummy_sctp_rs_test dummy_sctp_rs_test.cpp)
target_link_libraries(dummy_sctp_rs_test PRIVATE Catch2::Catch2WithMain rs_buffers util)
catch_discover_tests(dummy_sctp_rs_test)
