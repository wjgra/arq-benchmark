# Data packet unit tests
add_executable(data_packet_test data_packet_test.cpp)
target_link_libraries(data_packet_test PRIVATE Catch2::Catch2WithMain util arq_main)
catch_discover_tests(data_packet_test)

# Control packet unit tests
add_executable(control_packet_test control_packet_test.cpp)
target_link_libraries(control_packet_test PRIVATE Catch2::Catch2WithMain util arq_main)
catch_discover_tests(control_packet_test)
