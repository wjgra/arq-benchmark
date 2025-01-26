#ifndef _ARQ_RS_BUFFERS_GO_BACK_N_HPP_
#define _ARQ_RS_BUFFERS_GO_BACK_N_HPP_

#include "arq/common/resequencing_buffer.hpp"
#include "util/safe_queue.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace arq {
namespace rs {

class GoBackN : public ResequencingBuffer<GoBackN> {
public:
    GoBackN();

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    std::optional<DataPacket> do_getNextPacket();

private:
    // In Stop and Wait, only one packet is tracked at a time. If a received packet is not
    // the expected packet, it is ignored.
    // SequenceNumber expectedPacketSeqNum_;
    // // The packet to deliver to the output buffer.
    // std::optional<DataPacket> packetForDelivery_;
    // // Protect the next Packet and next packet sequence number
    // mutable std::mutex rsPacketMutex_;
    // // If no packet is ready for delivery, wait on this CV until one is.
    // std::condition_variable rsPacketCv_;
    // // Has an EoT packet been pushed to the OB?
    bool endOfTxPushed = false;
    util::SafeQueue<arq::DataPacket> shadowBuffer_;
    SequenceNumber expectedSN_ = FIRST_SEQUENCE_NUMBER;
};

} // namespace rs
} // namespace arq

#endif