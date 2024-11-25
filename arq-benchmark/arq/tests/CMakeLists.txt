# Data packet unit tests
add_executable(data_packet_test data_packet_test.cpp)
target_link_libraries(data_packet_test PRIVATE Catch2::Catch2WithMain lib_util lib_arq)
catch_discover_tests(data_packet_test)

# Control packet unit tests
add_executable(control_packet_test control_packet_test.cpp)
target_link_libraries(control_packet_test PRIVATE Catch2::Catch2WithMain lib_util lib_arq lib_arq_rtbufs)
catch_discover_tests(control_packet_test)
