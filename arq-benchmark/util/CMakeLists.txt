add_library(socket socket.cpp)

include_directories(${CMAKE_SOURCE_DIR}/arq-benchmark)

target_link_libraries(launcher socket)