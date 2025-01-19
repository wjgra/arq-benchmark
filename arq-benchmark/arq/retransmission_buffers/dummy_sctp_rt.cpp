#include "arq/retransmission_buffers/dummy_sctp_rt.hpp"

arq::rt::DummySCTP::DummySCTP() : RetransmissionBuffer{std::chrono::milliseconds(0)} /* Timeout is not needed for the dummy RT buffer */ {}

void arq::rt::DummySCTP::do_addPacket([[maybe_unused]] TransmitBufferObject&& packet) {}

std::optional<std::span<const std::byte>> arq::rt::DummySCTP::do_getPacketData()
{
    return std::nullopt;
}

bool arq::rt::DummySCTP::do_readyForNewPacket() const
{
    return true;
}

bool arq::rt::DummySCTP::do_packetsPending() const
{
    return false;
}

void arq::rt::DummySCTP::do_acknowledgePacket([[maybe_unused]] const SequenceNumber seqNum) {}
