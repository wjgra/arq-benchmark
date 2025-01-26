#include "arq/retransmission_buffers/stop_and_wait_rt.hpp"

#include "util/logging.hpp"

arq::rt::StopAndWait::StopAndWait(const std::chrono::microseconds timeout) : RetransmissionBuffer{timeout} {}

void arq::rt::StopAndWait::do_addPacket(arq::TransmitBufferObject&& packet)
{
    if (retransmitPacket_.has_value()) {
        throw ArqProtocolException("tried to add packet to S&W RT buffer but packet was already present");
    }
    retransmitPacket_ = packet;
}

std::optional<std::span<const std::byte>> arq::rt::StopAndWait::do_tryGetPacketSpan()
{
    if (retransmitPacket_.has_value() && isPacketTimedOut(retransmitPacket_.value())) {
        retransmitPacket_->updateLastTxTime();
        return retransmitPacket_->packet_.getReadSpan();
    }
    else {
        return std::nullopt;
    }
}

bool arq::rt::StopAndWait::do_readyForNewPacket() const
{
    return !retransmitPacket_.has_value();
}

bool arq::rt::StopAndWait::do_packetsPending() const
{
    return retransmitPacket_.has_value();
}

void arq::rt::StopAndWait::do_acknowledgePacket(const SequenceNumber ackSequenceNumber)
{
    if (!retransmitPacket_.has_value()) {
        util::logDebug("ACK received for SN {} but no packet stored for retransmission", ackSequenceNumber);
        return;
    }

    if (ackSequenceNumber == retransmitPacket_->info_.sequenceNumber_) {
        util::logDebug("ACK received for SN {}", ackSequenceNumber);
        retransmitPacket_ = std::nullopt;
    }
    else {
        util::logWarning(
            "ACK received for SN {}, but expected SN {}", ackSequenceNumber, retransmitPacket_->info_.sequenceNumber_);
    }
}
