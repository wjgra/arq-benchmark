# Add source files to util library
add_library(lib_util socket.cpp
                     address_info.cpp
                     endpoint.cpp)
target_link_libraries(launcher lib_util)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()