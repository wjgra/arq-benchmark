cmake_minimum_required(VERSION 3.22.1)

project(ARQ-Benchmark LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "Enable tests" ON)

find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)

include_directories(arq-benchmark)
add_subdirectory(arq-benchmark)