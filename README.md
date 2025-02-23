# Automatic Repeat Request (ARQ) Library
[![Unit tests (debug)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-debug.yml)
[![Unit tests (release)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-release.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/cmake-build-release.yml)
[![clang-format](https://github.com/wjgra/arq-benchmark/actions/workflows/clang-format.yml/badge.svg)](https://github.com/wjgra/arq-benchmark/actions/workflows/clang-format.yml)
## Overview
This library implements several [Automatic Repeat Request](https://en.wikipedia.org/wiki/Automatic_repeat_request) schemes for sending data over unreliable networks. Four sets of retransmission/resquencing buffers are provided:
- `arq::rt::StopAndWait` and `arq::rs::StopAndWait`: The simplest ARQ scheme. A packet is retransmitted until an acknowledgement is received. The next packet may then be transmitted.
- `arq::rt::GoBackN` and `arq::rs::GoBackN`: An ARQ scheme wherein the transmitter may transmit at most `N` packets ahead of the last packet that was acknowledged. The receiver is essentially the same as in stop-and-wait ARQ, with packets received out of sequence being discarded. Where `N=1`, this is equivalent to stop-and-wait.
- `arq::rt::SelectiveRepeat` and `arq::rs::SelectiveRepeat`: An ARQ scheme with a similar transmitter to Go-Back-N ARQ, but a receiver that may buffer up to `M` packets that are received out-of-order until they are needed. Where `M=1`, this is equivalent to Go-Back-N.
- `arq::rt::DummySCTP` and `arq::rs::DummySCTP`: A set of dummy buffers for sending data over a reliable SCTP connection, used as a control for the benchmarks.
## Project structure
A brief overview of the files this repo is shown below.
```
├── README.md (this file)
├── src
│   ├── arq
│   │   ├── common (packet structs; CRTP RS and RT buffers)
│   │   ├── resequencing_buffers (RS buffer template specialisations)
│   │   ├── retransmission_buffers (RT buffer template specialisations)
│   │   ├── receiver.hpp
│   │   └── transmitter.hpp
│   └── util (logging; socket abstractions)
├── format_all.sh
└── test_scripts
    ├── generate_plots.sh (runs a benchmark for multiple ARQ schemes and plots a comparison)
    ├── graph.py (parses logs from run.sh and plots the results)
    └── run.sh (runs a tmux'd launcher session with a transmitter and receiver)
```
## Dependencies and compilation
This project uses various features from C++20 and 23 (at time of writing, `<span>`, `<format>`, `<print>`, `<concepts>` and `<ranges>` are all used). It also uses the BSD sockets API for implementing `util::Socket`, so runs on Linux/WSL only. The launcher uses `boost::program_options` to handle the CLI arguments. The run script uses `tmux` to handle transmitter and receiver instances, and `tc` to simulate a lossy network connection between them. Unit tests depend on Catch2.

N.B. In order to use `tc`, you must ensure your Linux kernel has been compiled with the network emulator enabled.

Compilation of the library is handled by CMake and ninja:
```
mkdir build && cd build
cmake -GNinja ..
ninja
ninja test
```
## Benchmarks
The following benchmarks provide a basic comparison of the four sets of buffers mentioned above. In the future, I would be curious to compare it with [KCP](https://github.com/skywind3000/kcp/tree/master), which is a 'state of the art' ARQ implementation in C.

### Benchmark #1 - basic functionality in benign conditions
The aim of this benchmark is to demonstrate that all four schemes successfully deliver all packets when there is no packet loss and the interval between packet transmissions is larger than the round trip time.

**Parameters:** netem delay - `50ms`, netem loss - `random 0%`, packet transmission interval - `200 ms`, packets transmitted - `100`
```
sctp.log delays: mean 50.20031683168317 ms, median 50.197 ms, stdev 0.0218, min 50.144 ms, max 50.338 ms
snw.log delays: mean 50.195 ms, median 50.186 ms, stdev 0.043 ms, min 50.12 ms, max 50.513 ms
gbn.log delays: mean 50.199 ms, median 50.188 ms, stdev 0.050 ms, min 50.156 ms, max 50.592 ms
sr.log delays: mean 50.199 ms, median 50.19 ms, stdev 0.045 ms, min 50.147 ms, max 50.522 ms
```
![all_four_delay_50_loss_0_int_200_timeout_100](https://github.com/user-attachments/assets/bea6e508-5096-443a-a90e-23e62e31257b)

As expected, performance is broadly the same for all four schemes.
### Benchmark #2 - comparison of all schemes in lossy conditions
The aim of this (mildly amusing) benchmark is to demonstrate why the basic ARQ schemes (stop-and-wait in particular) are inadequate for usecases that require low latencies.

**Parameters:** netem delay - `50ms 10ms distribution normal`, netem loss - `random 1%`, packet transmission interval - `25 ms`, packets transmitted - `1000`
```
sctp.log delays: mean 69.931 ms, median 65.061 ms, stdev 37.365 ms, min 25.97 ms, max 370.466 ms
snw.log delays: mean 33522.937 ms, median 33466.984 ms, stdev 19326.603 ms, min 63.664 ms, max 67128.774 ms
gbn.log delays: mean 2815.560 ms, median 2348.146 ms, stdev 2043.208 ms, min 40.005 ms, max 8703.66 ms
sr.log delays: mean 52.714 ms, median 50.742 ms, stdev 17.252 ms, min 19.94 ms, max 167.729 ms
```
![all_four_delay_50_10_loss_1_int_25_timeout_100](https://github.com/user-attachments/assets/42ab8494-ab98-4fb4-969f-1d2b3673f3e8)

As is apparent, when packets are fed into the input buffer faster than acknowledgements can be received from the client, the delay when using stop-and-wait ARQ grows linearly. Since Go-Back-N ARQ is also limited in that the reciever cannot buffer packets that are received out of sequence, delays also grow in an unbounded manner here, albeit slightly less slowly. The more complex reciever functionality used in Selective Repeat ARQ yields similar performance to SCTP, which will be compared more closely in the next benchmark.
### Benchmark #3 - Selective Repeat ARQ versus SCTP 
The grand final! The aim of this benchmark is to compare the performance of Selective Repeat ARQ with SCTP, in the same network conditions as the previous benchmark.

**Parameters:** netem delay - `50ms 10ms distribution normal`, netem loss - `random 1%`, packet transmission interval - `25 ms`, packets transmitted - `1000`
```
sctp.log delays: mean 67.981 ms, median 64.382 ms, stdev 32.157 ms, min 22.775 ms, max 399.091 ms
sr.log delays: mean 53.286 ms, median 50.556 ms, stdev 19.233 ms, min 15.614 ms, max 253.945 ms
```
![sctp_src_delay_50_10_loss_1_int_25_timeout_100](https://github.com/user-attachments/assets/63554589-322d-4c28-9aa7-78c21d24ba91)

Here, it is apparent that Selective Repeat performs much better than SCTP. The average latencies are close to the theoretical best possible (~50 ms), and it seems from the spikes in the graph that SR ARQ recovers from losses with a lower impact on the latency of subsequent packets.

### Benchmark #4 - bonus
This benchmark was intended to compare SR ARQ with SCTP on a connections that loses packets in bursts, using the netem `state` loss model. However, this was too much for SCTP to handle without losing any packets - apparently it isn't as reliable a protocol as I had thought! In any case, results for SR ARQ are below.

**Parameters:** netem delay - `50ms 10ms distribution normal`, netem loss - `state 1% 5%`, packet transmission interval - `25 ms`, packets transmitted - `1000`
```
sr.log delays: mean 97.825 ms, median 53.78 ms, stdev 113.496 ms, min 22.103 ms, max 654.347 ms
```
![bonus_sr_delay_50_10_normal_loss_1_5_timeout_100_int_25](https://github.com/user-attachments/assets/7c524e98-223a-4643-adcb-3ce59ff80eb0)

Here, there are large latency spikes where we enter periods of burst losses, but latencies recover quickly when good service resumes.

N.B. In all four benchmarks, a window size of 100 is used for the GBN RS buffer and the SR RS and RT buffers. I did not measure overall network usage, however the general idea with ARQ is that you are gaining improved latency in exchange for incresed bandwidth usage - I expect SCTP's bandwidth usage to be much more efficient that my ARQ implementation.
