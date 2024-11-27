#include "arq/output_buffer.hpp"

#include "util/logging.hpp"

bool arq::OutputBuffer::addPacket(arq::DataPacket&& packet)
{
    const auto hdr = packet.getHeader();
    if (hdr.sequenceNumber_ == nextSequenceNumber_) {
        ++nextSequenceNumber_;
        if (packet.isEndOfTx()) {
            util::logInfo("Pushed End of Tx packet with SN {} to OB", hdr.sequenceNumber_);
        }
        else {
            util::logInfo("Pushed packet with SN {} to OB", hdr.sequenceNumber_);
        }
    }
    else {
        util::logDebug("OB rejected packet with SN {} (expected {})", hdr.sequenceNumber_, nextSequenceNumber_);
        return false;
    }
    arq::ReceiveBufferObject temp{.packet_ = std::move(packet), .rxTime_ = ClockType::now()};

    // Add packet to buffer
    outputPackets_.push(std::move(temp));
    return true;
}

arq::ReceiveBufferObject arq::OutputBuffer::getPacket()
{
    return outputPackets_.pop_wait();
}

std::optional<arq::ReceiveBufferObject> arq::OutputBuffer::tryGetPacket()
{
    return outputPackets_.try_pop();
}