#include "arq/resequencing_buffers/go_back_n_rs.hpp"

#include "util/logging.hpp"

arq::rs::GoBackN::GoBackN(SequenceNumber firstSeqNum) : nextSequenceNumber_{firstSeqNum} {}

std::optional<arq::SequenceNumber> arq::rs::GoBackN::do_addPacket(DataPacket&& packet)
{
    const auto& receivedPacket = packet;
    auto receivedSeqNum = receivedPacket.getHeader().sequenceNumber_;

    if (receivedSeqNum == nextSequenceNumber_) {
        canSendAcks_ = true; // When at least one packet received, we can send ACKs
        util::logDebug("Pushed packet with SN {} to shadow buffer", receivedSeqNum);

        ++nextSequenceNumber_;
        shadowBuffer_.push(std::move(packet));
        return receivedSeqNum;
    }
    else {
        // Reject packet - ACK last correctly received packet instead
        util::logDebug("Rejected packet with SN {}", receivedSeqNum);
        return canSendAcks_ ? std::make_optional(nextSequenceNumber_ - 1) : std::nullopt;
    }
}

bool arq::rs::GoBackN::do_packetsPending() const noexcept
{
    return !shadowBuffer_.empty();
}

std::optional<arq::DataPacket> arq::rs::GoBackN::do_getNextPacket()
{
    return shadowBuffer_.try_pop();
}
