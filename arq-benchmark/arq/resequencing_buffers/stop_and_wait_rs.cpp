#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"

#include "util/logging.hpp"

arq::rs::StopAndWait::StopAndWait(const std::chrono::microseconds finalTimeout) :
    expectedPacketSeqNum_{}, packetForDelivery_{std::nullopt}, nextAck_{std::nullopt}, finalTimeout_{finalTimeout}
{
}

void arq::rs::StopAndWait::do_addPacket(ReceiveBufferObject&& packet)
{
    std::scoped_lock<std::mutex> lock(rsPacketMutex_);
    // If the received packet is both the next packet expected and we haven't already received that packet,
    // add the packet to the buffer.
    auto hdr = packet.packet_.getHeader();
    if (hdr.sequenceNumber_ == expectedPacketSeqNum_ && packetForDelivery_.has_value() == false) {
        packetForDelivery_ = std::move(packet);
        util::logInfo("Added packet with SN {} to RS buffer", hdr.sequenceNumber_);
    }
    else {
        util::logDebug("RS buffer rejected packet with SN {}", hdr.sequenceNumber_);
    }
}

bool arq::rs::StopAndWait::do_packetsPending() const noexcept
{
    std::scoped_lock<std::mutex> lock(rsPacketMutex_);
    return packetForDelivery_.has_value();
}

arq::ReceiveBufferObject arq::rs::StopAndWait::do_getNextPacket()
{
    std::scoped_lock<std::mutex> lock(rsPacketMutex_);
    auto ret = std::move(packetForDelivery_.value());
    packetForDelivery_ = std::nullopt;
    ++expectedPacketSeqNum_;
    return ret;
}

std::optional<arq::SequenceNumber> arq::rs::StopAndWait::do_getNextAck()
{
    std::scoped_lock<std::mutex> lock(rsPacketMutex_);
    const auto ret = nextAck_;
    nextAck_ = std::nullopt;
    return ret;
}