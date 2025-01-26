#ifndef _ARQ_COMMON_SEQUENCE_NUMBER_HPP_
#define _ARQ_COMMON_SEQUENCE_NUMBER_HPP_

#include <cstdint>
#include <span>

namespace arq {
using SequenceNumber = uint16_t;

constexpr SequenceNumber FIRST_SEQUENCE_NUMBER = 0;

// WJG: to allow roll-over SNs
constexpr SequenceNumber MAX_SEQUENCE_NUMBER = __UINT16_MAX__;

// Serialises sequenceNumber to the buffer
bool serialiseSeqNum(const SequenceNumber sequenceNumber, std::span<std::byte> buffer) noexcept;

// Deserialises the buffer as a SequenceNumber
bool deserialiseSeqNum(SequenceNumber& sequenceNumber, std::span<const std::byte> buffer) noexcept;

} // namespace arq

#endif