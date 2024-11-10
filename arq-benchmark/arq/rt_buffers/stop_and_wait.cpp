#include "arq/rt_buffers/stop_and_wait.hpp"

#include "util/logging.hpp"

arq::StopAndWaitRTBuffer::StopAndWaitRTBuffer() {
    // Set a fixed timeout between retransmissions
    using namespace std::chrono_literals;
    timeout_ = 1000ms;
}

void arq::StopAndWaitRTBuffer::do_addPacket(arq::TransmitBufferObject&& packet) {
    retransmitPacket_ = std::move(packet);
}

std::optional<std::span<const std::byte>> arq::StopAndWaitRTBuffer::do_getPacketData() {

    if (currentPacketReadyForRT()) {
        retransmitPacket_.value().info_.txTime_ = arq::ClockType::now();
        return retransmitPacket_.value().packet_.getReadSpan();
    }
    else {
        return std::nullopt;
    }
}

bool arq::StopAndWaitRTBuffer::do_readyForNewPacket() const {
    return !retransmitPacket_.has_value();
}

bool arq::StopAndWaitRTBuffer::do_packetsPending() const {
    return retransmitPacket_.has_value();
}

void arq::StopAndWaitRTBuffer::do_acknowledgePacket(const SequenceNumber seqNum) {
    if (seqNum == retransmitPacket_.value().info_.sequenceNumber_) {
        retransmitPacket_ = std::nullopt;
    }
    else {
        util::logError("Ack received for SN {}, but expected SN {}", seqNum, retransmitPacket_.value().info_.sequenceNumber_);
    }
}

bool arq::StopAndWaitRTBuffer::currentPacketReadyForRT() const {
    if (retransmitPacket_.has_value()) {
        const auto now = arq::ClockType::now();
        const auto then = retransmitPacket_.value().info_.txTime_;
        const auto timeSinceLastTx = std::chrono::duration_cast<std::chrono::microseconds>(now - then);
        return timeSinceLastTx.count() > timeout_.count();
    }
    return false;   
}