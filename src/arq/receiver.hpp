#ifndef _ARQ_RECEIVER_HPP_
#define _ARQ_RECEIVER_HPP_

#include <atomic>
#include <memory>
#include <thread>
#include <type_traits>

#include "arq/common/arq_common.hpp"
#include "arq/common/control_packet.hpp"
#include "arq/common/conversation_id.hpp"
#include "arq/common/output_buffer.hpp"
#include "arq/common/resequencing_buffer.hpp"
#include "util/logging.hpp"
#include "util/safe_queue.hpp"

namespace arq {

template <typename T>
concept RSBuffer = std::is_base_of<ResequencingBuffer<T>, T>::value;

template <RSBuffer RSBufferType>
class Receiver {
public:
    Receiver(ConversationID id, TransmitFn txFn, ReceiveFn rxFn, std::unique_ptr<RSBufferType>&& rsBuffer_p) :
        id_{id},
        txFn_{txFn},
        rxFn_{rxFn},
        resequencingBuffer_{std::move(rsBuffer_p)},
        resequencingThread_{[this]() { return this->resequencingThread(); }},
        ackThread_{[this]() { return this->ackThread(); }},
        ackQueue_{},
        ackedEndOfTx_{false},
        endOfTxSn_{std::nullopt}
    {
    }

    ~Receiver()
    {
        if (resequencingThread_.joinable()) {
            resequencingThread_.join();
        }
        if (ackThread_.joinable()) {
            ackThread_.join();
        }
    }

    // If a packet is available, get the next packet from the output buffer.
    std::optional<ReceiveBufferObject> tryGetPacket() { return outputBuffer_.tryGetPacket(); }

private:
    std::optional<DataPacket> receivePacket() const
    {
        std::array<std::byte, MAX_TRANSMISSION_UNIT> recvBuffer;

        auto bytesRxed = rxFn_(recvBuffer);
        if (!bytesRxed.has_value() || bytesRxed == 0) {
            return std::nullopt;
        }
        assert(bytesRxed <= MAX_TRANSMISSION_UNIT);

        util::logDebug("Received {} bytes of data", bytesRxed.value());
        return arq::DataPacket(recvBuffer);
    }

    // The resequencing thread receives packets and determines whether they should be acked. Before
    // receiving a new packet, it checks whether any packets can be delivered to the output buffer.
    void resequencingThread()
    {
        while (!ackedEndOfTx_) {
            auto packet = receivePacket();

            // If a packet is received, feed it to the RS buffer and process any resulting ACKs.
            if (packet.has_value()) {
                auto pktHdr = packet->getHeader();
                util::logInfo("Received data packet with length {} and SN {}", pktHdr.length_, pktHdr.sequenceNumber_);

                // Record if EoT received
                if (packet->isEndOfTx()) {
                    endOfTxSn_ = pktHdr.sequenceNumber_;
                }

                auto ack = resequencingBuffer_->addPacket(std::move(packet.value()));
                if (ack.has_value()) {
                    ackQueue_.push(std::move(ack.value()));
                }
            }

            // Send any outstanding ACKs
            for (std::optional<DataPacket> packetForDelivery;
                 ((packetForDelivery = resequencingBuffer_->getNextPacket()) != std::nullopt);) {
                outputBuffer_.addPacket(std::move(packetForDelivery.value()));
            }
        }

        util::logInfo("Receiver resequencing thread exited");
    }

    void sendAck(SequenceNumber sequenceNumberToAck)
    {
        util::logInfo("Sending ACK for packet with SN {}", sequenceNumberToAck);
        arq::ControlPacket ctrlPkt = {.sequenceNumber_ = sequenceNumberToAck};
        std::array<std::byte, sizeof(arq::ControlPacket)> sendBuffer;

        if (ctrlPkt.serialise(sendBuffer)) {
            txFn_(sendBuffer);
            util::logDebug("Sent {} bytes", sendBuffer.size());
        }
        else {
            util::logError("Failed to serialise control packet");
        }
    }

    // The ACK thread transmits any ACKs stored in the ACK buffer.
    // It exits once the last ACK has been sent.
    void ackThread()
    {
        while (!ackedEndOfTx_) {
            auto nextToAck = ackQueue_.try_pop();
            if (nextToAck.has_value()) {
                sendAck(nextToAck.value());

                // Check if we've rx'd the last packet
                if (endOfTxSn_.load().has_value() && nextToAck.value() == endOfTxSn_.load().value()) {
                    util::logInfo("Sent ACK for End of Tx packet");
                    ackedEndOfTx_ = true;
                }
            }
        }
        util::logInfo("Receiver ACK thread exited");
    }

    // Identifies the current conversation (TO DO: issue #24)
    ConversationID id_;
    // Function pointer for raw data transmission
    TransmitFn txFn_;
    // Function pointer for raw data reception
    ReceiveFn rxFn_;
    // Store packets for delivery
    OutputBuffer outputBuffer_;
    // Store packets that have been received but not yet pushed to the output buffer
    std::unique_ptr<RSBufferType> resequencingBuffer_;
    // Thread handling packet reception and delivery to output buffer
    std::thread resequencingThread_;
    // Thread handling sending ACKs back to the transmitter
    std::thread ackThread_;

    util::SafeQueue<SequenceNumber> ackQueue_;
    // // If an EoT has been received, store the SN here
    // std::optional<SequenceNumber> endOfTxSeqNum_;
    // If an EoT has been received, store time of last packet reception
    // std::atomic<bool> pushedEndOfTx_;

    std::atomic<bool> ackedEndOfTx_;

    std::atomic<std::optional<SequenceNumber>> endOfTxSn_;
};

} // namespace arq

#endif
