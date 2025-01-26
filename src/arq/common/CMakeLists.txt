set(ARQ_COMMON_SRCS
    conversation_id.cpp
    control_packet.cpp
    data_packet.cpp
    input_buffer.cpp
    output_buffer.cpp
    sequence_number.cpp)

add_library(arq_common ${ARQ_COMMON_SRCS})

target_link_libraries(arq_main INTERFACE arq_common)