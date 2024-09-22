set(UTIL_SRCS socket.cpp
              address_info.cpp
              endpoint.cpp)

add_library(lib_util ${UTIL_SRCS})
target_link_libraries(launcher lib_util)

if (ENABLE_TESTS)
add_subdirectory(tests)
endif()
