#ifndef _ARQ_INPUT_BUFFER_HPP_
#define _ARQ_INPUT_BUFFER_HPP_

#include <chrono>
#include <vector>

#include "arq/data_packet.hpp"
#include "util/safe_queue.hpp"

namespace arq {

using SequenceNumber = uint16_t;

struct PacketInfo {
    // Time at which packet was added to the input buffer
    std::chrono::time_point<std::chrono::steady_clock> txTime_;
    // Sequence number assigned to packet by the input buffer
    SequenceNumber sequenceNumber_;
};

struct InputBufferObject {
    DataPacket packet_;
    PacketInfo info_;
};

class InputBuffer {
    util::SafeQueue<arq::DataPacket> inputPackets_;
};

}

#endif