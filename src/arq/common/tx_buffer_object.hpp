#ifndef _ARQ_TX_BUFFER_OBJECT_HPP_
#define _ARQ_TX_BUFFER_OBJECT_HPP_

#include <chrono>

#include "arq/common/arq_common.hpp"
#include "arq/common/data_packet.hpp"

namespace arq {

struct PacketInfo {
    // Time at which packet was first added to the buffer
    std::chrono::time_point<ClockType> firstTxTime_;
    // Time at which packet was last transmitted
    std::chrono::time_point<ClockType> lastTxTime_;
    // Sequence number assigned to packet by the input buffer
    SequenceNumber sequenceNumber_;
};

struct TransmitBufferObject {
    // Packet held in the InputBuffer
    DataPacket packet_;
    // Information for managing the state of the packet within the InputBuffer
    PacketInfo info_;

    void updateLastTxTime() { info_.lastTxTime_ = ClockType::now(); }

    bool isEndOfTx() const noexcept { return packet_.isEndOfTx(); }
};

} // namespace arq

#endif
