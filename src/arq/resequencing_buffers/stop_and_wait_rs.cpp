#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"

#include "util/logging.hpp"

arq::rs::StopAndWait::StopAndWait(SequenceNumber firstSeqNum) :
    expectedPacketSeqNum_{firstSeqNum}, packetForDelivery_{std::nullopt}
{
}
// WJG: we probably should not be dependent on the first SN here.

std::optional<arq::SequenceNumber> arq::rs::StopAndWait::do_addPacket(DataPacket&& packet)
{
    const auto receivedSequenceNumber = packet.getHeader().sequenceNumber_;

    if (packetForDelivery_.has_value()) {
        util::logInfo("Received packet with SN {} but RS buffer is full", receivedSequenceNumber);
        return std::nullopt;
    }

    if (receivedSequenceNumber == expectedPacketSeqNum_) {
        // If the expected packet is received, offer it for delivery
        packetForDelivery_ = std::move(packet);
        util::logInfo("Added packet with SN {} to RS buffer", receivedSequenceNumber);
        return receivedSequenceNumber;
    }
    else if (receivedSequenceNumber < expectedPacketSeqNum_) {
        // If an earlier than expected packet is received, one or more ACKs have been lost.
        util::logInfo("Packet {} received but RS buffer has already ACKed packet with SN {}",
                      receivedSequenceNumber,
                      expectedPacketSeqNum_ - 1);
        return receivedSequenceNumber;
    }
    else {
        util::logDebug(
            "RS buffer rejected packet with SN {} (expected {})", receivedSequenceNumber, expectedPacketSeqNum_);
    }

    return std::nullopt;
}

bool arq::rs::StopAndWait::do_packetsPending() const noexcept
{
    return packetForDelivery_.has_value();
}

// Removes the current packet from the RS buffer for delivery to the output buffer.
std::optional<arq::DataPacket> arq::rs::StopAndWait::do_getNextPacket()
{
    if (packetForDelivery_.has_value()) {
        auto ret = std::move(packetForDelivery_.value());
        packetForDelivery_ = std::nullopt;
        expectedPacketSeqNum_++;
        return ret;
    }
    return std::nullopt;
}
