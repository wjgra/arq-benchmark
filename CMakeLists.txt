cmake_minimum_required(VERSION 3.22.1)

project(ARQ-Benchmark LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(arq-benchmark)
add_subdirectory(arq-benchmark)