#ifndef _ARQ_SEQUENCE_NUMBER_HPP_
#define _ARQ_SEQUENCE_NUMBER_HPP_

#include <cstdint>
#include <span>

namespace arq {
using SequenceNumber = uint16_t;

constexpr SequenceNumber firstSequenceNumber = 1;

// Serialises sequenceNumber to the buffer
bool serialiseSeqNum(const SequenceNumber sequenceNumber, std::span<std::byte> buffer) noexcept;

// Deserialises the buffer as a SequenceNumber
bool deserialiseSeqNum(SequenceNumber& sequenceNumber, std::span<const std::byte> buffer) noexcept;

} // namespace arq

#endif