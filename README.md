# Automatic Repeat Request (ARQ) Library
[![Unit tests (debug)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml)
[![Unit tests (release)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-release.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-release.yml)
## Overview
This project is a work-in-progress, with further details TBC! In short, this repository relates to the [Automatic Repeat Request](https://en.wikipedia.org/wiki/Automatic_repeat_request) scheme for sending data over unreliable networks. The rough goal is to a) write a basic ARQ library for sending packets over UDP connections and b) compare this with both TCP connections and an existing ARQ library, [KCP](https://github.com/skywind3000/kcp/tree/master), in a series of performance benchmarks.
## Project structure
Below is a general overview of the repo as it currently stands. For each source file, `T` indicates that the file is covered by one or more dedicated unit tests, while `T*` indicates partial test coverage. 
```
arq-benchmark
 |-->arq
      |--> conversation_id.hpp/cpp
      |--> data_packet.hpp/cpp (T)
      |--> input_buffer.hpp/cpp - implementation ongoing
      |--> launcher.cpp
      |--> receiver.hpp/cpp - implementation ongoing
      |--> transmitter.hpp/cpp - implementation ongoing
 |-->util
      |--> address_info.hpp/cpp (T*)
      |--> endpoint.hpp.cpp (T)
      |--> logging.hpp
      |--> network_common.hpp
      |--> safe_queue.hpp
      |--> socket.hpp/cpp (T*)
test-scripts
 |-->run.sh (runs a tmux'd launcher session with a transmitter and receiver)
```
## Dependencies and compilation
The bulk of the implementation depends only on the BSD sockets API and the C++23 STL. The launcher uses Boost.ProgramOptions to handle the CLI. The run script uses `tmux` to handle transmitter and receiver instances, and `tc` to simulate a lossy network connection between them.

N.B. In order to use `tc` on WSL, you must recompile the Linux kernel with the network emulator enabled. This is fairly straightforward using `menuconfig`, but I may add brief instructions here in the future.

Compilation of the library is handled by CMake and ninja. As such, starting from the root of this repo, sample build commands may be as follows:
```
mkdir build
cd build
cmake -GNinja ..
ninja
ninja test
```
