#include "arq/common/control_packet.hpp"

bool arq::ControlPacket::serialise(std::span<std::byte> buffer) const noexcept
{
    return serialiseSeqNum(sequenceNumber_, buffer);
}

bool arq::ControlPacket::deserialise(std::span<const std::byte> buffer) noexcept
{
    return deserialiseSeqNum(sequenceNumber_, buffer);
}
