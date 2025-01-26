add_library(arq_main INTERFACE)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()

add_subdirectory(common)
add_subdirectory(retransmission_buffers)
add_subdirectory(resequencing_buffers)
