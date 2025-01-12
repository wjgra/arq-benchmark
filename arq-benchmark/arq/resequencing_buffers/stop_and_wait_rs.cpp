#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"

#include "util/logging.hpp"

arq::rs::StopAndWait::StopAndWait() : expectedPacketSeqNum_{FIRST_SEQUENCE_NUMBER}, packetForDelivery_{std::nullopt} {}

std::optional<arq::SequenceNumber> arq::rs::StopAndWait::do_addPacket(DataPacket&& packet)
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);

    // WJG: this slows things down - consider single-threaded implementation?
    util::logDebug("Waiting to add packet to RS");
    while (packetForDelivery_.has_value() && !endOfTxPushed) {
        rsPacketCv_.wait(lock);
    }

    // If EoT already pushed to OB, empty RS and indicate that no ACK should be Tx'd
    if (endOfTxPushed) {
        packetForDelivery_ = std::nullopt;
        return std::nullopt; // WJG: consider allowing further EoT ACKs to be sent
    }

    // If the received packet is both the next packet expected and we haven't already received that packet,
    // add the packet to the buffer.
    auto hdr = packet.getHeader();

    if (hdr.sequenceNumber_ == expectedPacketSeqNum_ || hdr.sequenceNumber_ == expectedPacketSeqNum_ + 1) {
        expectedPacketSeqNum_ = hdr.sequenceNumber_;

        packetForDelivery_ = std::move(packet);
        util::logInfo("Added packet with SN {} to RS buffer", hdr.sequenceNumber_);
        rsPacketCv_.notify_all(); // one?

        return hdr.sequenceNumber_;
    }
    else {
        util::logDebug("RS buffer rejected packet with SN {} (expected {} or {})",
                       hdr.sequenceNumber_,
                       expectedPacketSeqNum_,
                       expectedPacketSeqNum_ + 1);
    }

    return std::nullopt;
}

bool arq::rs::StopAndWait::do_packetsPending() const noexcept
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);
    return packetForDelivery_.has_value();
}

arq::DataPacket arq::rs::StopAndWait::do_getNextPacket()
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);

    util::logDebug("Waiting to get next packet from RS");
    while (!packetForDelivery_.has_value()) {
        rsPacketCv_.wait(lock);
    }

    auto ret = std::move(packetForDelivery_.value());

    if (ret.isEndOfTx()) {
        endOfTxPushed = true;
    }

    packetForDelivery_ = std::nullopt;
    rsPacketCv_.notify_all();
    return ret;
}
