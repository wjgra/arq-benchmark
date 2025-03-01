#ifndef _ARQ_COMMON_INPUT_BUFFER_HPP_
#define _ARQ_COMMON_INPUT_BUFFER_HPP_

#include <optional>

#include "arq/common/arq_common.hpp"
#include "arq/common/data_packet.hpp"
#include "arq/common/tx_buffer_object.hpp"

#include "util/safe_queue.hpp"

namespace arq {

class InputBuffer {
public:
    InputBuffer(SequenceNumber firstSeqNum = FIRST_SEQUENCE_NUMBER);
    // Submit a packet for transmission
    void addPacket(arq::DataPacket&& packet);
    // Get next packet for transmission from the buffer. If the buffer is empty,
    // wait until a packet is available
    TransmitBufferObject getPacket();
    // If a packet is available, get the next packet from the buffer.
    std::optional<TransmitBufferObject> tryGetPacket();

private:
    // Get information for populating data packet header
    PacketInfo getNextInfo();

    util::SafeQueue<TransmitBufferObject> inputPackets_;
    arq::SequenceNumber lastSequenceNumber_;
};

} // namespace arq

#endif
