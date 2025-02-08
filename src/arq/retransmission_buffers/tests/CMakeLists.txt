# Stop and wait RT buffer MUT
add_executable(stop_and_wait_rt_test stop_and_wait_rt_test.cpp)
target_link_libraries(stop_and_wait_rt_test PRIVATE Catch2::Catch2WithMain rt_buffers util)
catch_discover_tests(stop_and_wait_rt_test)

# Go-Back-N RT buffer MUT
add_executable(go_back_n_rt_test go_back_n_rt_test.cpp)
target_link_libraries(go_back_n_rt_test PRIVATE Catch2::Catch2WithMain rt_buffers util)
catch_discover_tests(go_back_n_rt_test)
