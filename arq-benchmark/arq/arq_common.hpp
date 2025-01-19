#ifndef _ARQ_COMMON_HPP_
#define _ARQ_COMMON_HPP_

#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <span>

namespace arq {

using ClockType = std::chrono::high_resolution_clock;

using TransmitFn = std::function<std::optional<size_t>(std::span<const std::byte> buffer)>;
using ReceiveFn = std::function<std::optional<size_t>(std::span<std::byte> buffer)>;

constexpr uint16_t packet_payload_length = 1000;

struct ArqProtocolException : public std::runtime_error {
    explicit ArqProtocolException(const std::string& what) : std::runtime_error(what){};
};

} // namespace arq

#endif
