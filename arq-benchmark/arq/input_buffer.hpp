#ifndef _ARQ_INPUT_BUFFER_HPP_
#define _ARQ_INPUT_BUFFER_HPP_

#include <chrono>
#include <optional>
#include <vector>

#include "arq/arq_common.hpp"
#include "arq/data_packet.hpp"
#include "util/safe_queue.hpp"

namespace arq {

struct PacketInfo {
    // Time at which packet was added to the input buffer
    std::chrono::time_point<ClockType> txTime_;
    // Sequence number assigned to packet by the input buffer
    SequenceNumber sequenceNumber_;
};

struct InputBufferObject {
    // Packet held in the InputBuffer
    DataPacket packet_;
    // Information for managing the state of the packet within the InputBuffer
    PacketInfo info_;
};

class InputBuffer {
public:
    // Submit a packet for transmission
    void addPacket(arq::DataPacket&& packet);
    // Get next packet for transmission from the buffer. If the buffer is empty, wait until a packet is available
    InputBufferObject getPacket();
    // If a packet is available, get the next packet from the buffer.
    std::optional<InputBufferObject> tryGetPacket();
private:
    // Get information for populating data packet header
    PacketInfo getNextInfo();

    util::SafeQueue<InputBufferObject> inputPackets_;
    arq::SequenceNumber lastSequenceNumber = 0;
};

}

#endif
