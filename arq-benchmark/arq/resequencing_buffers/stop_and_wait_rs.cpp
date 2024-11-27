#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"

#include "util/logging.hpp"

arq::rs::StopAndWait::StopAndWait(const std::chrono::microseconds finalTimeout) :
    expectedPacketSeqNum_{firstSequenceNumber}, packetForDelivery_{std::nullopt}, nextAck_{std::nullopt}, finalTimeout_{finalTimeout}
{
}

std::optional<SequenceNumber> arq::rs::StopAndWait::do_addPacket(ReceiveBufferObject&& packet)
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);
    // If the received packet is both the next packet expected and we haven't already received that packet,
    // add the packet to the buffer.
    auto hdr = packet.packet_.getHeader();
    if (hdr.sequenceNumber_ == expectedPacketSeqNum_ && packetForDelivery_.has_value() == false) {
        
        nextAck_ = expectedPacketSeqNum_++;
        packetForDelivery_ = std::move(packet);
        util::logInfo("Added packet with SN {} to RS buffer", hdr.sequenceNumber_);
        rsPacketCv_.notify_one();
    }
    else {
        util::logDebug("RS buffer rejected packet with SN {} (expected {})", hdr.sequenceNumber_, expectedPacketSeqNum_);
    }
}

bool arq::rs::StopAndWait::do_packetsPending() const noexcept
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);
    return packetForDelivery_.has_value();
}

arq::ReceiveBufferObject arq::rs::StopAndWait::do_getNextPacket()
{
    std::unique_lock<std::mutex> lock(rsPacketMutex_);

    while(packetForDelivery_.has_value() == false) {
        rsPacketCv_.wait(lock);
    }

    auto ret = std::move(packetForDelivery_.value());
    packetForDelivery_ = std::nullopt;
    return ret;
}

// std::optional<arq::SequenceNumber> arq::rs::StopAndWait::do_getNextAck()
// {
//     std::unique_lock<std::mutex> lock(rsPacketMutex_);
//     const auto ret = nextAck_;
//     nextAck_ = std::nullopt;
//     return ret;
// }