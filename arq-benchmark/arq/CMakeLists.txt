set(ARQ_SRCS
    conversation_id.cpp
    control_packet.cpp
    data_packet.cpp
    input_buffer.cpp
    output_buffer.cpp
    sequence_number.cpp)

set(ARQ_RT_BUFFER_SRCS
    retransmission_buffers/stop_and_wait_rt.cpp
    retransmission_buffers/dummy_sctp_rt.cpp
    retransmission_buffers/go_back_n_rt.cpp)  

set(ARQ_RS_BUFFER_SRCS
    resequencing_buffers/stop_and_wait_rs.cpp
    resequencing_buffers/dummy_sctp_rs.cpp
    resequencing_buffers/go_back_n_rs.cpp)

add_library(lib_arq ${ARQ_SRCS} ${ARQ_RT_BUFFER_SRCS} ${ARQ_RS_BUFFER_SRCS})

target_link_libraries(launcher lib_arq)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()
