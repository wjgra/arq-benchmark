#ifndef _ARQ_RT_BUFFERS_GO_BACK_N_HPP_
#define _ARQ_RT_BUFFERS_GO_BACK_N_HPP_

#include <cstdint>
#include <optional>
#include <vector>

#include "arq/common/retransmission_buffer.hpp"

namespace arq {
namespace rt {

class GoBackN : public RetransmissionBuffer<GoBackN> {
public:
    GoBackN(uint16_t windowSize, const std::chrono::microseconds timeout);

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(TransmitBufferObject&& packet);
    std::optional<std::span<const std::byte>> do_getPacketDataSpan();
    bool do_readyForNewPacket() const;
    bool do_packetsPending() const;
    void do_acknowledgePacket(const SequenceNumber seqNum);

private:
    // WJG should these things be in the CRTP interface?
    // A packet is retransmitted only when the timeout has expired without
    // reception of an ACK.
    const std::chrono::microseconds timeout_;

    const uint16_t windowSize_;

    // WJG: This should be moved out of the class
    struct CircularBuffer {
        // A buffer to hold packets for retransmission
        std::vector<std::optional<TransmitBufferObject>> buffer_;
        // Index into buffer_ corresponding to the start of the circular buffer
        size_t startIdx_;
        //
        SequenceNumber nextSequenceNumberToAck_;
    } slidingWindow_;
};

} // namespace rt
} // namespace arq

#endif
