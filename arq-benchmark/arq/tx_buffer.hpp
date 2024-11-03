#ifndef _ARQ_TX_BUFFER_HPP_
#define _ARQ_TX_BUFFER_HPP_

#include <chrono>

#include "arq/arq_common.hpp"
#include "arq/data_packet.hpp"

namespace arq {

struct PacketInfo {
    // Time at which packet was added to the input buffer
    std::chrono::time_point<ClockType> txTime_;
    // Sequence number assigned to packet by the input buffer
    SequenceNumber sequenceNumber_;
};

struct TransmitBufferObject {
    // Packet held in the InputBuffer
    DataPacket packet_;
    // Information for managing the state of the packet within the InputBuffer
    PacketInfo info_;
};

}

#endif
