include_directories(${CMAKE_SOURCE_DIR}/arq-benchmark)

# Add source files to util library
add_library(lib_util socket.cpp
                     address_info.cpp
                     endpoint.cpp)
target_link_libraries(launcher lib_util)