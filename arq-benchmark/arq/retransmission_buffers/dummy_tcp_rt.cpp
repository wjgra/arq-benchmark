#include "arq/retransmission_buffers/dummy_tcp_rt.hpp"

arq::rt::DummyTCP::DummyTCP() {}

void arq::rt::DummyTCP::do_addPacket([[maybe_unused]] TransmitBufferObject&& packet) {}

std::optional<std::span<const std::byte>> arq::rt::DummyTCP::do_getPacketData()
{
    return std::nullopt;
}

bool arq::rt::DummyTCP::do_readyForNewPacket() const
{
    return true;
}

bool arq::rt::DummyTCP::do_packetsPending() const
{
    return false;
}

void arq::rt::DummyTCP::do_acknowledgePacket([[maybe_unused]] const SequenceNumber seqNum) {}
