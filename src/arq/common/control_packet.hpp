#ifndef _ARQ_COMMON_CONTROL_PACKET_HPP_
#define _ARQ_COMMON_CONTROL_PACKET_HPP_

#include "arq/common/sequence_number.hpp"

namespace arq {

// A control packet sent from arq::Receiver to arq::Transmitter to acknowledge
// packets
struct ControlPacket {
    // The sequence number being acknowledged
    SequenceNumber sequenceNumber_;

    // Serialises the current contents of the ControlPacket to the buffer
    bool serialise(std::span<std::byte> buffer) const noexcept;
    // Deserialises the buffer into the ControlPacket
    bool deserialise(std::span<const std::byte> buffer) noexcept;
};

} // namespace arq

#endif