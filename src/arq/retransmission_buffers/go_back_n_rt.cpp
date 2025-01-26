#include "arq/retransmission_buffers/go_back_n_rt.hpp"
#include "util/logging.hpp"

arq::rt::GoBackN::GoBackN(uint16_t windowSize, const std::chrono::microseconds timeout) :
    RetransmissionBuffer{timeout},
    timeout_{timeout},
    windowSize_{windowSize},
    slidingWindow_{.buffer_ = std::vector<std::optional<TransmitBufferObject>>(windowSize, std::nullopt),
                   .startIdx_ = 0,
                   .nextSequenceNumberToAck_ = FIRST_SEQUENCE_NUMBER}
{
}

// Add packet to last space in buffer
// WJG to make mutex more precise. Consider mutating/non-mutating lock
void arq::rt::GoBackN::do_addPacket(TransmitBufferObject&& packet)
{
    size_t pkt_idx = slidingWindow_.startIdx_;
    do {
        // Fill first empty slot with packet
        if (slidingWindow_.buffer_[pkt_idx] == std::nullopt) {
            slidingWindow_.buffer_[pkt_idx] = packet;
            return;
        }

        pkt_idx = (pkt_idx + 1) % windowSize_;
    } while (pkt_idx != slidingWindow_.startIdx_);
}

// Consider whether this should return a vector? No, because we need to update timing
std::optional<std::span<const std::byte>> arq::rt::GoBackN::do_tryGetPacketSpan()
{
    // Transmit untransmitted pkts in window
    // Transmit lost packets in window

    // Loop over packets in window - consider an iterator
    size_t pkt_idx = slidingWindow_.startIdx_;
    do {
        auto& this_pkt = slidingWindow_.buffer_[pkt_idx];

        if (this_pkt.has_value() && isPacketTimedOut(this_pkt.value())) {
            util::logDebug("Retransmit packet at idx {}", pkt_idx);
            this_pkt->info_.lastTxTime_ = arq::ClockType::now();
            return this_pkt->packet_.getReadSpan();
        }

        pkt_idx = (pkt_idx + 1) % windowSize_;
    } while (pkt_idx != slidingWindow_.startIdx_);
    return std::nullopt;
}

/* A new packet may be added if there is space in the circular buffer. */
bool arq::rt::GoBackN::do_readyForNewPacket() const
{
    // If the last packet in the window is empty, then there is space for at least one new packet
    const auto lastPacket_idx = (slidingWindow_.startIdx_ + windowSize_ - 1) % windowSize_;
    return slidingWindow_.buffer_[lastPacket_idx] == std::nullopt;
}

bool arq::rt::GoBackN::do_packetsPending() const
{
    size_t pkt_idx = slidingWindow_.startIdx_;
    do {
        const auto& this_pkt = slidingWindow_.buffer_[pkt_idx];
        if (this_pkt.has_value()) {
            return true;
        }
        pkt_idx = (pkt_idx + 1) % windowSize_; // simplify with iterator
    } while (pkt_idx != slidingWindow_.startIdx_);
    return false;
}

// The alg is actually simpler than I thought. You only send ACKs for in order packtets
void arq::rt::GoBackN::do_acknowledgePacket(const SequenceNumber seqNum)
{ // wjg rename to acked sn

    for (SequenceNumber sn = slidingWindow_.nextSequenceNumberToAck_; sn <= seqNum; ++sn) {
        slidingWindow_.buffer_[slidingWindow_.startIdx_++] = std::nullopt;
        slidingWindow_.startIdx_ = slidingWindow_.startIdx_ % windowSize_; // %= ?
    }

    if (seqNum >= slidingWindow_.nextSequenceNumberToAck_) {
        slidingWindow_.nextSequenceNumberToAck_ = seqNum + 1;
    }

    /*     if (seqNum >= slidingWindow_.nextSequenceNumberToAck_ && seqNum < slidingWindow_.nextSequenceNumberToAck_ +
       windowSize_) { auto packetToAck_idx = (seqNum - slidingWindow_.nextSequenceNumberToAck_ +
       slidingWindow_.startIdx_ + windowSize_ - 1) % windowSize_; slidingWindow_.buffer_[lastPacket_idx] = std::nullopt;
       // check this whole fn
        }

        while (slidingWindow_.buffer_[slidingWindow_.startIdx_] == std::nullopt) { // and not reached end...
            slidingWindow_.startIdx_++;
            slidingWindow_.startIdx_ = slidingWindow_.startIdx_ % windowSize_;
            slidingWindow_.nextSequenceNumberToAck_++;
        } */
}