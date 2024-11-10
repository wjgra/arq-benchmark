set(ARQ_SRCS
    conversation_id.cpp
    data_packet.cpp
    input_buffer.cpp
    receiver.cpp)

add_library(lib_arq ${ARQ_SRCS})

set(ARQ_RT_BUFFER_SRCS
    rt_buffers/stop_and_wait.cpp)

add_library(lib_arq_rtbufs ${ARQ_RT_BUFFER_SRCS})
target_link_libraries(lib_arq lib_arq_rtbufs)

target_link_libraries(launcher lib_arq)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()
