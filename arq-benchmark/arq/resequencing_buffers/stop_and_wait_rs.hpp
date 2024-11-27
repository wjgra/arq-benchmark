#ifndef _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_

#include "arq/resequencing_buffer.hpp"

#include <condition_variable>
#include <mutex>

namespace arq {
namespace rs {

class StopAndWait : public ResequencingBuffer<StopAndWait> {
public:
    StopAndWait(const std::chrono::microseconds finalTimeout);

    // Standard functions required by RetransmissionBuffer CRTP interface
    void do_addPacket(ReceiveBufferObject&& packet);
    bool do_packetsPending() const noexcept;
    ReceiveBufferObject do_getNextPacket();
    std::optional<SequenceNumber> do_getNextAck();

private:
    // In Stop and Wait, only one packet is tracked at a time. If a received packet is not
    // the expected packet, it is ignored.
    SequenceNumber expectedPacketSeqNum_;
    // The packet to deliver to the output buffer.
    std::optional<ReceiveBufferObject> packetForDelivery_;
    // Protect the next Packet and next packet sequence number
    mutable std::mutex rsPacketMutex_;
    // If no packet is ready for delivery, wait on this CV until one is.
    std::condition_variable rsPacketCv_;
    // The next ACK for delivery to the transmitter.
    std::optional<SequenceNumber> nextAck_;
    // Once an ACK for the EoT packet has been transmitted, if no packets are received for the
    // duration of the timeout, terminate the RS buffer. WJG: how to link to receiver?
    std::chrono::microseconds finalTimeout_;
};

} // namespace rs
} // namespace arq

#endif