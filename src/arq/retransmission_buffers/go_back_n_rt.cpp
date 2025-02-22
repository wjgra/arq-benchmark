#include "arq/retransmission_buffers/go_back_n_rt.hpp"
#include "util/logging.hpp"

arq::rt::GoBackN::GoBackN(const uint16_t windowSize,
                          const std::chrono::microseconds timeout,
                          const SequenceNumber firstSeqNum) :
    RetransmissionBuffer{timeout},
    windowSize_{windowSize},
    buffer_{std::vector<std::optional<TransmitBufferObject>>(windowSize, std::nullopt)},
    startIdx_{0},
    nextToAck_{firstSeqNum},
    packetsInBuffer_{0}
{
}

// Add a packet to the next space in the circular buffer
void arq::rt::GoBackN::do_addPacket(TransmitBufferObject&& packet)
{
    size_t pkt_idx = startIdx_;
    do {
        if (buffer_[pkt_idx] == std::nullopt) {
            buffer_[pkt_idx] = packet;
            packetsInBuffer_++;
            return;
        }

        pkt_idx = (pkt_idx + 1) % windowSize_;
    } while (pkt_idx != startIdx_);
    throw ArqProtocolException("tried to add packet to Go-Back-N RT buffer, but buffer was full");
}

std::optional<std::span<const std::byte>> arq::rt::GoBackN::do_tryGetPacketSpan()
{
    if (!do_packetsPending()) {
        return std::nullopt;
    }

    // Retransmit the earliest packet that has not timed out.
    size_t pkt_idx = startIdx_;
    do {
        auto& this_pkt = buffer_[pkt_idx];

        if (this_pkt.has_value() && isPacketTimedOut(this_pkt.value())) {
            util::logDebug("Retransmit packet at idx {}", pkt_idx);
            this_pkt->info_.lastTxTime_ = arq::ClockType::now();
            return this_pkt->packet_.getReadSpan();
        }

        pkt_idx = (pkt_idx + 1) % windowSize_;
    } while (pkt_idx != startIdx_);
    return std::nullopt;
}

// A new packet may be added if there is space in the circular buffer.
bool arq::rt::GoBackN::do_readyForNewPacket() const noexcept
{
    return packetsInBuffer_ < windowSize_;
}

bool arq::rt::GoBackN::do_packetsPending() const noexcept
{
    return packetsInBuffer_ > 0;
}

// in GBN ARQ, ACKs are only sent for in order packets.
void arq::rt::GoBackN::do_acknowledgePacket(const SequenceNumber ackedSeqNum)
{
    if (ackedSeqNum >= nextToAck_ + windowSize_) {
        util::logError(
            "Tried to ACK packet with SN {}, which is outside of possible range for GBN RT buffer starting at {} of size {}",
            ackedSeqNum,
            nextToAck_,
            windowSize_);
        return;
    }

    if (!do_packetsPending()) {
        util::logError("No packets to ACK in GBN RT buffer");
        return;
    }

    // Since packets are only ACK'd in order, any packet before the ACK is also ACK'd
    for (int i = 0; i < windowSize_; ++i) {
        const auto pkt_idx = (startIdx_ + i) % windowSize_;

        // WJG: Here we assume non-wrapping SNs
        if (nextToAck_ + i <= ackedSeqNum) {
            buffer_[pkt_idx] = std::nullopt;
            packetsInBuffer_--;
        }
    }

    // Update the circular buffer info based on the new 'next packet to ACK'
    if (ackedSeqNum >= nextToAck_) {
        startIdx_ += (ackedSeqNum + 1 - nextToAck_) % windowSize_;
        nextToAck_ = ackedSeqNum + 1;
    }
}
