#ifndef _ARQ_RT_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RT_BUFFERS_STOP_AND_WAIT_HPP_

#include <mutex>
#include <optional>

#include "arq/retransmission_buffer.hpp"

namespace arq {
namespace rt {

class StopAndWait : public RetransmissionBuffer<StopAndWait> {
public:
    StopAndWait(const std::chrono::microseconds timeout, const bool adaptiveTimeout);

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(TransmitBufferObject&& packet);
    std::optional<std::span<const std::byte>> do_getPacketData();
    bool do_readyForNewPacket() const;
    bool do_packetsPending() const;
    void do_acknowledgePacket(const SequenceNumber seqNum);
private:
    bool currentPacketReadyForRT() const;

    // In Stop and Wait, only one packet is stored for retransmission at a time.
    std::optional<TransmitBufferObject> retransmitPacket_ = std::nullopt;
    // Control access to the retransmit packet
    mutable std::mutex rtPacketMutex_;
    // A packet is retransmitted only when the timeout has expired without
    // reception of an ACK.
    std::chrono::microseconds timeout_;
    // Update the timeout based on the measured round trip time.
    const bool adaptiveTimeout_;
};

}
}

#endif
