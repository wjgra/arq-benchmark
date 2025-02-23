#include "arq/resequencing_buffers/selective_repeat_rs.hpp"

#include "util/logging.hpp"

arq::rs::SelectiveRepeat::SelectiveRepeat(const uint16_t windowSize, SequenceNumber firstSeqNum) :
    windowSize_{windowSize},
    buffer_{std::vector<std::optional<DataPacket>>(windowSize, std::nullopt)},
    earliestExpected_{firstSeqNum}
{
}

inline int arithmetic_modulo(const int a, const int b)
{
    const int out = a % b;
    return out >= 0 ? out : out + b;
}

/* Checks whether any packets are now in sequence and can be forwarded to the OB. Forwards
 * any such packets and updated RS buffer tracking information. */
void arq::rs::SelectiveRepeat::updateBuffer()
{
    size_t pkt_idx = startIdx_;
    size_t newStartIdx = startIdx_;

    // Iterate until the first missing packet is found
    do {
        if (!buffer_[pkt_idx].has_value()) {
            newStartIdx = pkt_idx;
            util::logDebug("First missing packet at index {}", pkt_idx);
            break;
        }
        else {
            // Push in-order packets to shadow buffer
            util::logDebug("Push packet at index {} to shadow buffer", pkt_idx);
            shadowBuffer_.push(std::move(buffer_[pkt_idx].value()));
            buffer_[pkt_idx] = std::nullopt;

            // Update number of packets stored in the RS buffer sliding window
            assert(packetsInBuffer_ > 0);
            packetsInBuffer_--;
        }

        pkt_idx = (pkt_idx + 1) % windowSize_;
    } while (pkt_idx != startIdx_);

    // Update sliding window tracking info
    if (newStartIdx != startIdx_) {
        if (newStartIdx > startIdx_) {
            earliestExpected_ += newStartIdx - startIdx_;
        }
        else {
            // The start index has wrapped around the end of the sliding window
            earliestExpected_ += newStartIdx - startIdx_ + windowSize_;
        }

        startIdx_ = newStartIdx;
        util::logDebug("Earliest expected packet now has SN {}", earliestExpected_);
    }
}

std::optional<arq::SequenceNumber> arq::rs::SelectiveRepeat::do_addPacket(DataPacket&& packet)
{
    /* Accept packets in interval [earliest, earliest + window) */
    const auto& receivedPacket = packet;
    auto receivedSeqNum = receivedPacket.getHeader().sequenceNumber_;

    if (receivedSeqNum < earliestExpected_ || receivedSeqNum >= earliestExpected_ + windowSize_) {
        util::logDebug("Rejected packet with SN {} (earliest expected is {})", receivedSeqNum, earliestExpected_);
    }
    else {
        canSendAcks_ = true; // When at least one packet has been received, we can send ACKs

        const size_t insertionIdx = arithmetic_modulo(startIdx_ + receivedSeqNum - earliestExpected_, windowSize_);

        util::logDebug("Received packet with SN {}, try to insert at index {}", receivedSeqNum, insertionIdx);

        // If the packet hasn't already been received, add it to the RS buffer
        if (!buffer_[insertionIdx].has_value()) {
            util::logDebug("Added packet with SN {} to resequencing buffer", receivedSeqNum);
            buffer_[insertionIdx] = std::move(packet);
            packetsInBuffer_++;

            updateBuffer();
        }
        util::logDebug("Packet with SN {} already present in RS buffer", receivedSeqNum);
    }

    return canSendAcks_ ? std::make_optional(earliestExpected_ - 1) : std::nullopt;
}

bool arq::rs::SelectiveRepeat::do_packetsPending() const noexcept
{
    return packetsInBuffer_ > 0 || !shadowBuffer_.empty();
}

std::optional<arq::DataPacket> arq::rs::SelectiveRepeat::do_getNextPacket()
{
    return shadowBuffer_.try_pop();
}
