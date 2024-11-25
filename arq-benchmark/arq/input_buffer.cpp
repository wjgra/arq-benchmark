#include "arq/input_buffer.hpp"

#include "util/logging.hpp"

void arq::InputBuffer::addPacket(arq::DataPacket&& packet)
{
    arq::TransmitBufferObject temp{.packet_ = std::forward<arq::DataPacket>(packet), .info_ = getNextInfo()};
    // Add info to packet header
    temp.packet_.updateSequenceNumber(temp.info_.sequenceNumber_);

    util::logDebug("Adding packet with SN {} to IB", temp.info_.sequenceNumber_);

    // Add packet to buffer
    inputPackets_.push(std::move(temp));
}

arq::TransmitBufferObject arq::InputBuffer::getPacket()
{
    return inputPackets_.pop_wait();
}

std::optional<arq::TransmitBufferObject> arq::InputBuffer::tryGetPacket()
{
    return inputPackets_.try_pop();
}

arq::PacketInfo arq::InputBuffer::getNextInfo()
{
    const auto currentTime = ClockType::now();
    return PacketInfo{
        .firstTxTime_ = currentTime, .lastTxTime_ = currentTime, .sequenceNumber_ = ++lastSequenceNumber_};
}
