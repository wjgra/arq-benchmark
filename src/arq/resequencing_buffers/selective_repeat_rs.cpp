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

// Check whether any packets are now in sequence and can be forwarded to the OB.
void arq::rs::SelectiveRepeat::updateBufferIndices()
{
    size_t pkt_idx = startIdx_;
    size_t newStartIdx = startIdx_;
    // Iterate until the first missing pkt
    do {
        if (!buffer_[pkt_idx].has_value()) {
            newStartIdx = pkt_idx;
            util::logError("First missing at {}", pkt_idx);
            break;
        }
        else {
            // push to shadow buffer and decrement packets in buffer
            util::logError("Push from {}", pkt_idx);
            shadowBuffer_.push(std::move(buffer_[pkt_idx].value()));
            buffer_[pkt_idx] = std::nullopt;
            assert(packetsInBuffer_ > 0);
            packetsInBuffer_--;
        }

        pkt_idx = (pkt_idx + 1) % windowSize_;
    } while (pkt_idx != startIdx_);

    // WJG to clean up
    if (newStartIdx != startIdx_) { // what if it's behind?
        util::logError("New idx {}, old idx {}", newStartIdx, startIdx_);
        // log
        if (newStartIdx > startIdx_) {
            earliestExpected_ += newStartIdx - startIdx_;
        }
        else { // we've wrapped around
            earliestExpected_ += newStartIdx - startIdx_ + windowSize_;
        }

        startIdx_ = newStartIdx;
        util::logError("earliest expected {}", earliestExpected_);
    }
}

std::optional<arq::SequenceNumber> arq::rs::SelectiveRepeat::do_addPacket(DataPacket&& packet)
{
    /* Accept packets in interval [earliest, earliest + window) */
    const auto& receivedPacket = packet;
    auto receivedSeqNum = receivedPacket.getHeader().sequenceNumber_;

    // std::println("Buffer state");
    // for (size_t i = 0 ; i < windowSize_ ; ++i) {
    //     std::print("{} ", buffer_[i].has_value() ? buffer_[i]->getHeader().sequenceNumber_ : -1);
    // }
    // std::println("");

    util::logError("Rxed {}, earliest {}", receivedSeqNum, earliestExpected_);

    if (receivedSeqNum < earliestExpected_ || receivedSeqNum >= earliestExpected_ + windowSize_) {
        util::logDebug("Rejected packet with SN {}", receivedSeqNum);
    }
    else {
        canSendAcks_ = true; // When at least one packet received, we can send ACKs

        const size_t insertionIdx = arithmetic_modulo(startIdx_ + receivedSeqNum - earliestExpected_, windowSize_);

        util::logError("Rxed {}, insert at {}", receivedSeqNum, insertionIdx);

        // If the packet hasn't already been received, add it to the RS buffer
        if (!buffer_[insertionIdx].has_value()) {
            util::logDebug("Added packet with SN {} to resequencing buffer", receivedSeqNum);
            buffer_[insertionIdx] = std::move(packet);
            packetsInBuffer_++;

            updateBufferIndices();

            //     // Update the start of the buffer
            //     size_t pkt_idx = startIdx_;
            //     size_t newStartIdx = startIdx_;
            //     do {
            //         // Iterate until the first missing pkt
            //         if (!buffer_[pkt_idx].has_value()) {
            //             newStartIdx = pkt_idx;
            //             util::logError("First missing at {}", pkt_idx);
            //             break;
            //         }
            //         else {
            //             // push to shadow buffer and decrement packets in buffer
            //             util::logError("Push from {}", pkt_idx);
            //             shadowBuffer_.push(std::move(buffer_[pkt_idx].value()));
            //             buffer_[pkt_idx] = std::nullopt;
            //             assert(packetsInBuffer_ > 0);
            //             packetsInBuffer_--;
            //         }

            //         pkt_idx = (pkt_idx + 1) % windowSize_;
            //     } while (pkt_idx != startIdx_);

            //     // WJG to clean up
            //     if (newStartIdx > startIdx_) { // what if it's behind?
            //         util::logError("New idx {}, old idx {}", newStartIdx, startIdx_);
            //         // log
            //         earliestExpected_ += newStartIdx - startIdx_;
            //         startIdx_ = newStartIdx;
            //         util::logError("earliest expected {}", earliestExpected_);
            //     }
        }
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
