#ifndef _ARQ_RS_BUFFERS_GO_BACK_N_HPP_
#define _ARQ_RS_BUFFERS_GO_BACK_N_HPP_

#include "arq/common/resequencing_buffer.hpp"
#include "util/safe_queue.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace arq {
namespace rs {

/* In Go-Back-N ARQ, the resequencing buffer is a sliding window of size one. Fundamentally, it has similar function to
 * the Stop-and-Wait RS buffer. As compared to Selective Repeat ARQ, whilst this scheme is slightly less effective, it
 * has the benefit of reducing receiver complexity. */
class GoBackN : public ResequencingBuffer<GoBackN> {
public:
    GoBackN(SequenceNumber firstSeqNum = FIRST_SEQUENCE_NUMBER);

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    std::optional<DataPacket> do_getNextPacket();

private:
    // In Go Back N, only one packet is tracked at a time on the recieve side.
    // If a received packet is not the expected packet, it is ignored, and an
    // ACK is instead sent for the last correctly received packet.

    // Store packets received here for delivery to the output buffer.
    util::SafeQueue<arq::DataPacket> shadowBuffer_;

    // The next sequence number expected by the RS buffer.
    SequenceNumber nextSequenceNumber_;

    // ACKs can only be sent once at least one packet has been correctly received.
    bool canSendAcks_ = false;
};

} // namespace rs
} // namespace arq

#endif