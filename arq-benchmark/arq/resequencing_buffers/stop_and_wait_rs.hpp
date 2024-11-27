#ifndef _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_

#include "arq/resequencing_buffer.hpp"

#include <mutex>

namespace arq {
namespace rt {

class StopAndWait : public ResequencingBuffer<StopAndWait> {
public:
    StopAndWait(const std::chrono::microseconds finalTimeout);

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(ReceiveBufferObject&& packet);
    bool do_packetsPending() const noexcept;
    ReceiveBufferObject do_getNextPacket() std::optional<SequenceNumber> do_getNextAck();

private:
    // In Stop and Wait, only one packet is tracked at a time. If a received packet is not
    // the expected packet, it is ignored.
    SequenceNumber nextPacket_;
    // Protect the SN of the nextPacket_
    std::mutex rsPacketMutex_;
    // Once an ACK for the EoT packet has been transmitted, if no packets are received for the
    // duration of the timeout, terminate the RS buffer. WJG: how to link to receiver?
    std::chrono::microseconds finalTimemout_;
};

} // namespace rt
} // namespace arq

#endif