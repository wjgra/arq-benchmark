#ifndef _ARQ_RT_BUFFERS_DUMMY_SCTP_HPP_
#define _ARQ_RT_BUFFERS_DUMMY_SCTP_HPP_

#include "arq/retransmission_buffer.hpp"

namespace arq {
namespace rt {

class DummySCTP : public RetransmissionBuffer<DummySCTP> {
public:
    DummySCTP();

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(TransmitBufferObject&& packet);
    std::optional<std::span<const std::byte>> do_getPacketData();
    bool do_readyForNewPacket() const;
    bool do_packetsPending() const;
    void do_acknowledgePacket(const SequenceNumber seqNum);
};

} // namespace rt
} // namespace arq

#endif
