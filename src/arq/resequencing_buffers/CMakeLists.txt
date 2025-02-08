set(ARQ_RS_BUFFER_SRCS
    stop_and_wait_rs.cpp
    dummy_sctp_rs.cpp
    go_back_n_rs.cpp
    selective_repeat_rs.cpp)

add_library(rs_buffers ${ARQ_RS_BUFFER_SRCS})

target_link_libraries(rs_buffers arq_common)
target_link_libraries(arq_main INTERFACE rs_buffers)

add_subdirectory(tests)
