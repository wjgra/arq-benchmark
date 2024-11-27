#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"

#include "util/logging.hpp"

arq::rs::StopAndWait::StopAndWait(const std::chrono::microseconds finalTimeout) :
    expectedPacketSeqNum_{firstSequenceNumber},
    packetForDelivery_{std::nullopt},
    /* nextAck_{std::nullopt},  */ finalTimeout_{finalTimeout}
{
}

std::optional<arq::SequenceNumber> arq::rs::StopAndWait::do_addPacket(DataPacket&& packet)
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);

    //  std::unique_lock<std::mutex> lock(rsPacketMutex_);
    util::logDebug("Waiting to add packet to RS");
    while (packetForDelivery_.has_value() == true && endOfTxPushed == false) {
        rsPacketCv_.wait(lock);
    }

    if (endOfTxPushed == true) {
        return std::nullopt;
    }

    // If the received packet is both the next packet expected and we haven't already received that packet,
    // add the packet to the buffer.
    auto hdr = packet.getHeader();

    if (hdr.sequenceNumber_ == expectedPacketSeqNum_ || hdr.sequenceNumber_ == expectedPacketSeqNum_ + 1) {
        expectedPacketSeqNum_ = hdr.sequenceNumber_; // update

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

    // if (hdr.sequenceNumber_ == expectedPacketSeqNum_ && packetForDelivery_.has_value() == false) {
    //     packetForDelivery_ = std::move(packet);
    //     util::logInfo("Added packet with SN {} to RS buffer", hdr.sequenceNumber_);
    //     rsPacketCv_.notify_one();
    //     return expectedPacketSeqNum_++; // wjg; what about next SN?
    // }
    // else {

    // }
    return std::nullopt;
}

bool arq::rs::StopAndWait::do_packetsPending() const noexcept
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);
    return packetForDelivery_.has_value();
    // return false; // always false?
}

arq::DataPacket arq::rs::StopAndWait::do_getNextPacket()
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);

    util::logDebug("Waiting to get next packet from RS");
    while (packetForDelivery_.has_value() == false) {
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

// std::optional<arq::SequenceNumber> arq::rs::StopAndWait::do_getNextAck()
// {
//     std::unique_lock<std::mutex> lock(rsPacketMutex_);
//     const auto ret = nextAck_;
//     nextAck_ = std::nullopt;
//     return ret;
// }