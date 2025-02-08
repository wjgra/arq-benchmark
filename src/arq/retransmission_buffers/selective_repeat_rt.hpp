#ifndef _ARQ_RT_BUFFERS_SELECTIVE_REPEAT_HPP_
#define _ARQ_RT_BUFFERS_SELECTIVE_REPEAT_HPP_

#include <cstdint>
#include <optional>
#include <vector>

#include "arq/common/retransmission_buffer.hpp"

namespace arq {
namespace rt {

class SelectiveRepeat : public RetransmissionBuffer<SelectiveRepeat> {
public:
    SelectiveRepeat(const uint16_t windowSize,
                    const std::chrono::microseconds timeout,
                    const SequenceNumber firstSeqNum = FIRST_SEQUENCE_NUMBER);

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(TransmitBufferObject&& packet);
    std::optional<std::span<const std::byte>> do_tryGetPacketSpan();
    bool do_readyForNewPacket() const noexcept;
    bool do_packetsPending() const noexcept;
    void do_acknowledgePacket(const SequenceNumber ackedSeqNum);

private:
    // The window defines the maximum number of packets that can be in the RT buffer.
    const uint16_t windowSize_;
    // A circular buffer holding the packets for retransmission.
    std::vector<std::optional<TransmitBufferObject>> buffer_;
    // The index within the buffer of the earliest packet.
    size_t startIdx_;
    // The next SN to acknowledge - this corresponds to the earliest packet in the buffer.
    SequenceNumber nextToAck_;
    // The current number of packets in the buffer. By keeping track of this, we can cut down on
    // unneccessary buffer iteration.
    size_t packetsInBuffer_;
};

} // namespace rt
} // namespace arq

#endif
