set(ARQ_SRCS
    conversation_id.cpp
    data_packet.cpp
    input_buffer.cpp
    sequence_number.cpp)

add_library(lib_arq ${ARQ_SRCS})

set(ARQ_RT_BUFFER_SRCS
    retransmission_buffers/stop_and_wait_rt.cpp)

add_library(lib_arq_rtbufs ${ARQ_RT_BUFFER_SRCS})
target_link_libraries(lib_arq lib_arq_rtbufs)

# set(ARQ_RS_BUFFER_SRCS
#     resequencing_buffers/stop_and_wait_rs.cpp)

# add_library(lib_arq_rsbufs ${ARQ_RS_BUFFER_SRCS})
# target_link_libraries(lib_arq lib_arq_rsbufs)

target_link_libraries(launcher lib_arq)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()
