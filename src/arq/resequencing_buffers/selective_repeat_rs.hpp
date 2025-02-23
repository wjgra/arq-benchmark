#ifndef _ARQ_RS_BUFFERS_SELECTIVE_REPEAT_HPP_
#define _ARQ_RS_BUFFERS_SELECTIVE_REPEAT_HPP_

#include "arq/common/resequencing_buffer.hpp"
#include "util/safe_queue.hpp"

#include <cstdint>

namespace arq {
namespace rs {

/* In Selective Repeat ARQ, the resequencing buffer is a sliding window, similar to the
 * one used for retransmission. Unlike GBN and SNW ARQ, packets recieved out of order
 * are buffered until needed.  */
class SelectiveRepeat : public ResequencingBuffer<SelectiveRepeat> {
public:
    SelectiveRepeat(const uint16_t windowSize, SequenceNumber firstSeqNum = FIRST_SEQUENCE_NUMBER);

    // Standard functions required by ResequencingBuffer CRTP interface
    std::optional<SequenceNumber> do_addPacket(DataPacket&& packet);
    bool do_packetsPending() const noexcept;
    std::optional<DataPacket> do_getNextPacket();

private:
    void updateBufferIndices();

    // The window defines the maximum number of packets that can be in the RS buffer.
    const uint16_t windowSize_;

    // A circular buffer holding the packets for resequencing.
    std::vector<std::optional<arq::DataPacket>> buffer_;

    // The index within the buffer of the earliest packet.
    size_t startIdx_ = 0;

    // The SN corresponding to the earliest packet in the RS buffer.
    SequenceNumber earliestExpected_;

    // Packets in the circular buffer.
    size_t packetsInBuffer_ = 0;

    // Store packets received here for delivery to the output buffer.
    util::SafeQueue<arq::DataPacket> shadowBuffer_;

    // ACKs can only be sent once at least one packet has been correctly received.
    bool canSendAcks_ = false;
};

} // namespace rs
} // namespace arq

#endif
