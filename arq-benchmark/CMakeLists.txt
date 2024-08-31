include_directories(${CMAKE_SOURCE_DIR}/arq-benchmark)

# Executables
add_executable(launcher launcher.cpp)

# Boost
cmake_policy(SET CMP0054 NEW)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS program_options REQUIRED)
if (Boost_FOUND)
    target_include_directories(launcher PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(launcher ${Boost_LIBRARIES})
endif()

# Subdirectories
add_subdirectory(util)
