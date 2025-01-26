set(ARQ_RT_BUFFER_SRCS
    stop_and_wait_rt.cpp
    dummy_sctp_rt.cpp
    go_back_n_rt.cpp)

add_library(rt_buffers ${ARQ_RT_BUFFER_SRCS})

target_link_libraries(rt_buffers arq_common)
target_link_libraries(arq_main INTERFACE rt_buffers)