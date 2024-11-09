#ifndef _ARQ_RT_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RT_BUFFERS_STOP_AND_WAIT_HPP_

#include "arq/retransmission_buffer.hpp"

namespace arq {

class StopAndWaitRTBuffer : public RetransmissionBuffer<StopAndWaitRTBuffer> {
public:
    // Standard functions required by RetransmissionBuffer interface
    void do_addPacket(TransmitBufferObject&& packet);
    std::optional<std::span<const std::byte>> do_getPacketData() const;
    bool do_readyForNewPacket() const;
    bool do_packetsPending() const;
    void do_acknowledgePacket(const SequenceNumber seqNum);

    std::optional<TransmitBufferObject> retransmitPacket_ = std::nullopt;
};

}

#endif
