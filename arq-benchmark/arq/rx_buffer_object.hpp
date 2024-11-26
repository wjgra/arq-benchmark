#ifndef _ARQ_RX_BUFFER_OBJECT_HPP_
#define _ARQ_RX_BUFFER_OBJECT_HPP_

#include <chrono>

#include "arq/arq_common.hpp"
#include "arq/data_packet.hpp"

namespace arq {

struct ReceiveBufferObject {
    // Packet held in the OutputBuffer
    DataPacket packet_;

    // The time at which the packet was received
    std::chrono::time_point<ClockType> rxTime_;
};

} // namespace arq

#endif