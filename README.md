# Automatic Repeat Request (ARQ) Library
[![Unit tests (debug)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml)
[![Unit tests (release)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-release.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-release.yml)
[![clang-format](https://github.com/wjgra/arq-benchmark/actions/workflows/clang-format.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/clang-format.yml)
## Overview
This library implements several [Automatic Repeat Request](https://en.wikipedia.org/wiki/Automatic_repeat_request) schemes for sending data over unreliable networks. 
## Project structure
A brief overview of the files this repo is shown below.
```
├── README.md (this file)
├── arq-benchmark (top-level source folder)
│   ├── arq (packet structs; transmitter/receiver defintions; CRTP RS and RT buffers)
│   │   ├── resequencing_buffers (RS buffer template instantiations)
│   │   └── retransmission_buffers (RT buffer template instantiations)
│   └── util
├── format_all.sh
└── test_scripts
    ├── generate_plots.sh (runs a benchmark for multiple ARQ schemes and plots a comparison)
    ├── graph.py (parses logs from run.sh and plots the results)
    └── run.sh (runs a tmux'd launcher session with a transmitter and receiver)
```
## Dependencies and compilation
This project uses various features from C++20 and 23 (at time of writing, `<span>`, `<format>`, `<print>` and `<ranges>` are all used). It also uses the BSD sockets API for implementing `util::Socket`, so runs on Linux/WSL only. The launcher uses to handle the CLI arguments. The run script uses `tmux` to handle transmitter and receiver instances, and `tc` to simulate a lossy network connection between them. Unit tests depend on Catch2.

N.B. In order to use `tc`, you must ensure your Linux kernel has been compiled with the network emulator enabled.

Compilation of the library is handled by CMake and ninja:
```
mkdir build && cd build
cmake -GNinja ..
ninja
ninja test
```
## Benchmarks (TBC)
The goal is to compare the performance of 'stop-and-wait', 'go-back-N' and 'selective-repeat' retranmission schemes, using an SCTP connection as the control. I would also be curious to compare it with [KCP](https://github.com/skywind3000/kcp/tree/master).
