#include "arq/retransmission_buffer.hpp"

#include "util/logging.hpp"

void arq::RetransmissionBuffer::addPacket(arq::TransmitBufferObject&& packet) {
    // S&W implementation
    retransmitPacket_ = packet;
}

std::optional<std::span<const std::byte>> arq::RetransmissionBuffer::getRetransmitPacketData() {
    // S&W implementation
    if (packetAcked) {
        return std::nullopt;
    }
    else {
        return retransmitPacket_.packet_.getReadSpan();
    }
}

void arq::RetransmissionBuffer::acknowledgePacket(const SequenceNumber seqNum) {
    // S&W implementation
    if (seqNum == retransmitPacket_.info_.sequenceNumber_) {
        packetAcked = true;
    }
    else {
        util::logError("Ack received for SN {}, but expected SN {}", seqNum, retransmitPacket_.info_.sequenceNumber_);
    }
}

bool arq::RetransmissionBuffer::packetsPending() {
    // S&W implementation
    return !packetAcked;
}
