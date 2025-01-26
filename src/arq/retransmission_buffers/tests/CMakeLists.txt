# Stop and wait RT buffer MUT
add_executable(stop_and_wait_rt_test stop_and_wait_rt_test.cpp)
target_link_libraries(stop_and_wait_rt_test PRIVATE Catch2::Catch2WithMain rt_buffers util)
catch_discover_tests(stop_and_wait_rt_test)
