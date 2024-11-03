#ifndef _ARQ_COMMON_HPP_ 
#define _ARQ_COMMON_HPP_

#include <chrono>
#include <cstdint>
#include <functional>
#include <span>

namespace arq {

using SequenceNumber = uint16_t;
using ClockType = std::chrono::steady_clock;

using TransmitFn = std::function<std::optional<size_t>(std::span<const std::byte> buffer)>;
using ReceiveFn = std::function<std::optional<size_t>(std::span<std::byte> buffer)>;

}

#endif