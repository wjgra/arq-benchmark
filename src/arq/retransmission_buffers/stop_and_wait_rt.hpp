#ifndef _ARQ_RT_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RT_BUFFERS_STOP_AND_WAIT_HPP_

#include <optional>

#include "arq/common/retransmission_buffer.hpp"

namespace arq {
namespace rt {

class StopAndWait : public RetransmissionBuffer<StopAndWait> {
public:
    StopAndWait(const std::chrono::microseconds timeout);

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(TransmitBufferObject&& packet);
    std::optional<std::span<const std::byte>> do_tryGetPacketSpan();
    bool do_readyForNewPacket() const;
    bool do_packetsPending() const;
    void do_acknowledgePacket(const SequenceNumber ackSequenceNumber);

private:
    // In Stop and Wait, only one packet is stored for retransmission at a time.
    std::optional<TransmitBufferObject> retransmitPacket_ = std::nullopt;
};

} // namespace rt
} // namespace arq

#endif
