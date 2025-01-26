# Endpoint unit tests
add_executable(endpoint_test endpoint_test.cpp)
target_link_libraries(endpoint_test PRIVATE Catch2::Catch2WithMain
                                            util)
catch_discover_tests(endpoint_test)
