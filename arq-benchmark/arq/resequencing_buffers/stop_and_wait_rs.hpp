#ifndef _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_
#define _ARQ_RS_BUFFERS_STOP_AND_WAIT_HPP_

#include "arq/resequencing_buffer.hpp"

#include <condition_variable>
#include <mutex>

namespace arq {
namespace rs {

class StopAndWait : public ResequencingBuffer<StopAndWait> {
public:
    StopAndWait();

    // Standard functions required by RetransmissionBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    DataPacket do_getNextPacket();
    // std::optional<SequenceNumber> do_getNextAck();

private:
    // In Stop and Wait, only one packet is tracked at a time. If a received packet is not
    // the expected packet, it is ignored.
    SequenceNumber expectedPacketSeqNum_;
    // The packet to deliver to the output buffer.
    std::optional<DataPacket> packetForDelivery_;
    // Protect the next Packet and next packet sequence number
    mutable std::mutex rsPacketMutex_;
    // If no packet is ready for delivery, wait on this CV until one is.
    std::condition_variable rsPacketCv_;
    // Has an EoT packet been pushed to the OB?
    bool endOfTxPushed = false;
};

} // namespace rs
} // namespace arq

#endif