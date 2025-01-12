#include "arq/retransmission_buffers/stop_and_wait_rt.hpp"

#include "util/logging.hpp"

arq::rt::StopAndWait::StopAndWait(const std::chrono::microseconds timeout, const bool adaptiveTimeout) :
    timeout_{timeout}, adaptiveTimeout_{adaptiveTimeout}
{
}

void arq::rt::StopAndWait::do_addPacket(arq::TransmitBufferObject&& packet)
{
    std::scoped_lock<std::mutex> lock(rtPacketMutex_);
    retransmitPacket_ = std::move(packet);
}

std::optional<std::span<const std::byte>> arq::rt::StopAndWait::do_getPacketData()
{
    if (currentPacketReadyForRT()) {
        std::scoped_lock<std::mutex> lock(rtPacketMutex_);
        retransmitPacket_->info_.lastTxTime_ = arq::ClockType::now();
        return retransmitPacket_->packet_.getReadSpan();
    }
    else {
        return std::nullopt;
    }
}

bool arq::rt::StopAndWait::do_readyForNewPacket() const
{
    std::scoped_lock<std::mutex> lock(rtPacketMutex_);
    return !retransmitPacket_.has_value();
}

bool arq::rt::StopAndWait::do_packetsPending() const
{
    std::scoped_lock<std::mutex> lock(rtPacketMutex_);
    return retransmitPacket_.has_value();
}

void arq::rt::StopAndWait::do_acknowledgePacket(const SequenceNumber seqNum)
{
    std::scoped_lock<std::mutex> lock(rtPacketMutex_);
    if (!retransmitPacket_.has_value()) {
        util::logDebug("Ack received for SN {} but no packet stored for retransmission", seqNum);
        return;
    }

    if (seqNum == retransmitPacket_->info_.sequenceNumber_) {
        // Update timeout based on round trip time
        if (adaptiveTimeout_) {
            const auto now = arq::ClockType::now();
            const auto then = retransmitPacket_->info_.firstTxTime_;
            const auto timeSinceFirstTx = std::chrono::duration_cast<std::chrono::microseconds>(now - then);

            // Due to lost packets, timeSinceFirstTx may actually be greater than the true RTT. Damp
            // the update to the timeout by taking the mean of the old timeout and the measured 'RTT'.

            /* Packet loss means that timeSinceFirstTx may actually be greater than the true RTT. Dampen
             * any updates to the timeout by taking the mean of the old timout and the measured 'RTT', and
             * cap it to a maximum of 100ms. */
            using namespace std::chrono_literals;
            constexpr auto maxTimeout = 100'000us;
            const auto newTimeout = std::min((timeSinceFirstTx + timeout_) / 2, maxTimeout);
            util::logInfo("Measured RTT: {} Old timeout: {} New timeout: {} (max {})",
                          timeSinceFirstTx,
                          timeout_,
                          newTimeout,
                          maxTimeout);
            timeout_ = newTimeout;
        }

        // Ack packet
        retransmitPacket_ = std::nullopt;
    }
    else {
        util::logWarning(
            "Ack received for SN {}, but expected SN {}", seqNum, retransmitPacket_->info_.sequenceNumber_);
    }
}

bool arq::rt::StopAndWait::currentPacketReadyForRT() const
{
    std::scoped_lock<std::mutex> lock(rtPacketMutex_);
    if (retransmitPacket_.has_value()) {
        const auto now = arq::ClockType::now();
        const auto then = retransmitPacket_->info_.lastTxTime_;
        const auto timeSinceLastTx = std::chrono::duration_cast<std::chrono::microseconds>(now - then);
        return timeSinceLastTx.count() > timeout_.count();
    }
    return false;
}