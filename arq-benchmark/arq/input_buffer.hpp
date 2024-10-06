#ifndef _ARQ_INPUT_BUFFER_HPP_
#define _ARQ_INPUT_BUFFER_HPP_

#include <chrono>
#include <optional>
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
    auto getNextInfo() {
        return PacketInfo{.txTime_ = std::chrono::steady_clock::now(), .sequenceNumber_ = ++lastSequenceNumber};
    }
public:
    void addPacket(arq::DataPacket&& packet) {
        arq::InputBufferObject temp{.packet_ = std::forward<arq::DataPacket>(packet),
                                    .info_ = getNextInfo()};
        inputPackets_.push(std::move(temp));
    }

    InputBufferObject getPacket() {
        return inputPackets_.pop_wait();
    }

    std::optional<InputBufferObject> tryGetPacket() {
        return inputPackets_.try_pop();
    }
private:
    util::SafeQueue<InputBufferObject> inputPackets_;
    arq::SequenceNumber lastSequenceNumber = 0;
};

}

#endif