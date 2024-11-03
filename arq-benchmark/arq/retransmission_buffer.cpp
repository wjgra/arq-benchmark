#include "arq/retransmission_buffer.hpp"

void arq::RetransmissionBuffer::addPacket(arq::TransmitBufferObject&& packet) {
    (void)packet;
    assert(false);
}

std::optional<std::span<std::byte>> arq::RetransmissionBuffer::getRetransmitPacketData() {
    assert(false);
}

void arq::RetransmissionBuffer::acknowledgePacket(const SequenceNumber seqNum) {
    (void)seqNum;
    assert(false);
}

bool arq::RetransmissionBuffer::packetsPending() {
    assert(false);
}
