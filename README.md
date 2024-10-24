# arq-benchmark
[![Unit tests (debug)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml)
[![Build ARQ Benchmarks with CMake](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build.yml)

Further details TBC! In short, this project relates to the [Automatic Repeat Request](https://en.wikipedia.org/wiki/Automatic_repeat_request) method for sending data over unreliable networks. The rough goal is to a) write a basic ARQ library for sending packets over UDP connections and b) compare this with both TCP connections and an existing ARQ library, [KCP](https://github.com/skywind3000/kcp/tree/master), in a series of performance benchmarks. 
