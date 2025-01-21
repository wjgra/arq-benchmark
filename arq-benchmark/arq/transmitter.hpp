#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>

#include "arq/arq_common.hpp"
#include "arq/conversation_id.hpp"
#include "arq/input_buffer.hpp"
#include "arq/retransmission_buffer.hpp"

#include "util/logging.hpp"

namespace arq {

template <typename T>
concept RTBuffer = std::is_base_of<RetransmissionBuffer<T>, T>::value;

template <RTBuffer RTBufferType>
class Transmitter {
public:
    Transmitter(ConversationID id, TransmitFn txFn, ReceiveFn rxFn, std::unique_ptr<RTBufferType>&& rtBuffer_p) :
        id_{id},
        txFn_{txFn},
        rxFn_{rxFn},
        retransmissionBuffer_{std::move(rtBuffer_p)},
        transmitThread_{[this]() { return this->transmitThread(); }},
        ackThread_{[this]() { return this->ackThread(); }},
        endOfTxSeqNum_{std::nullopt},
        endOfTxAcked_{false}
    {
    }

    ~Transmitter()
    {
        if (transmitThread_.joinable()) {
            transmitThread_.join();
        }
        if (ackThread_.joinable()) {
            ackThread_.join();
        }
        util::logDebug("Transmitter exiting");
    }

    void sendPacket(arq::DataPacket&& packet) { inputBuffer_.addPacket(std::move(packet)); }

private:
    // Transmits data using the transmit function.
    void transmitPacketData(auto dataToTx) const
    {
        auto result = txFn_(dataToTx);
        if (result.has_value()) {
            util::logDebug("Successfully transmitted {} bytes", result.value());
        }
        else {
            util::logError("Transmit function failed!");
        }
    }

    // Attempts to transmit a packet from the RT buffer, returns true if the RT is non-empty.
    bool attemptPacketRetransmission()
    {
        auto packetSpanToReTx = retransmissionBuffer_->getPacketDataSpan();
        bool packetAvailable = packetSpanToReTx.has_value();
        if (packetAvailable) {
            DataPacketHeader hdr;
            hdr.deserialise(packetSpanToReTx.value());

            util::logInfo("Retransmitting packet with SN {} and length {}", hdr.sequenceNumber_, hdr.length_);
            transmitPacketData(packetSpanToReTx.value());
        }
        return packetAvailable;
    }

    // Attempts to transmit a new packet from the input buffer, returns true if the IB is non-empty.
    bool attemptNewPacketTransmission()
    {
        auto newPkt = inputBuffer_.tryGetPacket();
        bool packetAvailable = newPkt.has_value();
        if (packetAvailable) {
            if (newPkt->isEndOfTx()) {
                util::logInfo("Transmitter received end of EndofTx from input buffer");
                endOfTxSeqNum_ = newPkt->info_.sequenceNumber_;
            }

            util::logInfo("Transmitting packet with SN {} and adding to retransmission buffer",
                          newPkt->info_.sequenceNumber_);
            transmitPacketData(newPkt->packet_.getReadSpan());

            retransmissionBuffer_->addPacket(std::move(newPkt.value()));
        }
        return packetAvailable;
    }

    // Passes every sequence number from the ACK queue to the RT buffer for acknowledgement.
    void processAckQueue()
    {
        for (std::optional<SequenceNumber> snToAck;
             !endOfTxAcked_ && ((snToAck = ackQueue_.try_pop()) != std::nullopt);) {
            if (snToAck == endOfTxSeqNum_) {
                endOfTxAcked_ = true;
            }
            else {
                retransmissionBuffer_->acknowledgePacket(snToAck.value());
            }
        }
    }

    // The transmit thread handles transmission and retransmission of all packets. It continues
    // running until an ACK corresponding to an end of transmission (EoT) packet has been received.
    void transmitThread()
    {
        util::logInfo("Transmitter Tx thread started");

        while (!endOfTxAcked_) {
            if (!attemptPacketRetransmission()) {
                attemptNewPacketTransmission();
            }

            // Every ACK is processed as quickly as possible to reduce unneccessary transmissions.
            processAckQueue();
        }

        // WJG to investigate the 'if no packet is in transmission' clause from Wikipedia
        util::logInfo("Transmitter Tx thread exited");
    }

    // The acknowledgement thread handles ACKs received at the transmitter from the receiver. It runs until
    // an ACK is received for an EoT packet.
    void ackThread()
    {
        util::logInfo("Transmitter ACK thread started");

        while (!endOfTxAcked_) {
            // If an ACK is recieved, add it to the ACK queue.
            std::array<std::byte, arq::MAX_TRANSMISSION_UNIT> recvBuffer;
            auto receivedBytes = rxFn_(recvBuffer);
            if (receivedBytes > 0) {
                arq::SequenceNumber receivedSequenceNumber;
                if (arq::deserialiseSeqNum(receivedSequenceNumber, recvBuffer)) {
                    util::logInfo("Received ACK for SN {}", receivedSequenceNumber);

                    // WJG: we shouldn't have to move here - check queue implementation
                    ackQueue_.push(std::move(receivedSequenceNumber));
                }
                else {
                    util::logWarning("Received packet that is too short to be an ACK");
                }
            }
            else {
                util::logWarning("Transmitter receive function failed");
            }
        }

        util::logInfo("Transmitter ACK thread exited");
    }

    // Identifies the current conversation (TO DO: issue #24)
    ConversationID id_;
    // Function pointer for raw data transmission
    TransmitFn txFn_;
    // Function pointer for raw data reception
    ReceiveFn rxFn_;
    // Store packets for transmission that are yet to be transmitted
    InputBuffer inputBuffer_;
    // Store packets that have been transmitted but not acknowledged, and so may
    // require retransmission
    std::unique_ptr<RTBufferType> retransmissionBuffer_;
    // Thread handling data packet transmission and retransmission
    std::thread transmitThread_;
    // Thread handling reception of ACKs for processing by the transmit thread
    std::thread ackThread_;
    // Keeps track of ACKs received at the transmitter
    util::SafeQueue<SequenceNumber> ackQueue_; // wjg: arguably, this should be a priority queue
    // If an EoT has been received, store the sequence number
    std::optional<SequenceNumber> endOfTxSeqNum_;
    // Has an EoT packet been transmitted and acknowledged?
    std::atomic<bool> endOfTxAcked_;
};

} // namespace arq

#endif
