cmake_policy(SET CMP0054 NEW)
find_package(Boost 1.74.0 COMPONENTS boost_program_options)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${CMAKE_SOURCE_DIR}/arq-benchmark)

add_executable(launcher launcher.cpp)
target_link_libraries(launcher boost_program_options)

add_subdirectory(util)