#include "arq/sequence_number.hpp"

#include <netdb.h>
#include <cstring>

bool arq::serialiseSeqNum(const SequenceNumber sequenceNumber, std::span<std::byte> buffer) noexcept
{
    static_assert(sizeof(SequenceNumber) == 2);
    if (buffer.size() < sizeof(SequenceNumber)) {
        return false;
    }
    else {
        const uint16_t temp = htons(sequenceNumber);
        std::memcpy(buffer.data(), &temp, sizeof(SequenceNumber));
        return true;
    }
}

bool arq::deserialiseSeqNum(SequenceNumber& sequenceNumber, std::span<const std::byte> buffer) noexcept
{
    static_assert(sizeof(SequenceNumber) == 2);
    if (buffer.size() < sizeof(SequenceNumber)) {
        return false;
    }
    else {
        uint16_t temp;
        std::memcpy(&temp, buffer.data(), sizeof(SequenceNumber));
        sequenceNumber = ntohs(temp);
        return true;
    }
}