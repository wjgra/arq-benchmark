#ifndef _ARQ_OUTPUT_BUFFER_HPP_
#define _ARQ_OUTPUT_BUFFER_HPP_

#include <optional>

#include "arq/rx_buffer_object.hpp"
#include "util/safe_queue.hpp"

namespace arq {

class OutputBuffer {
public:
    // Submit a packet for output - reject if not the next in sequence
    bool addPacket(arq::DataPacket&& packet);

    // Get next packet for output from the buffer. If the buffer is empty, wait until a packet is available
    ReceiveBufferObject getPacket();

    // If a packet is available, get the next packet from the buffer.
    std::optional<ReceiveBufferObject> tryGetPacket();

private:
    // Packets for output from the receiver
    util::SafeQueue<ReceiveBufferObject> outputPackets_;
    // The next SN to be accepted for addition to the output buffer
    arq::SequenceNumber nextSequenceNumber_ = firstSequenceNumber;
};

} // namespace arq

#endif