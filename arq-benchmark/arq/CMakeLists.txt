set(ARQ_SRCS
    conversation_id.cpp
    data_packet.cpp
    input_buffer.cpp
    receiver.cpp
    retransmission_buffer.cpp
    transmitter.cpp)

add_library(lib_arq ${ARQ_SRCS})
target_link_libraries(launcher lib_arq)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()
