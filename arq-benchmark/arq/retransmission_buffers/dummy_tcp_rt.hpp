#ifndef _ARQ_RT_BUFFERS_DUMMY_TCP_HPP_
#define _ARQ_RT_BUFFERS_DUMMY_TCP_HPP_

#include "arq/retransmission_buffer.hpp"

namespace arq {
namespace rt {

class DummyTCP : public RetransmissionBuffer<DummyTCP> {
public:
    DummyTCP();

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
