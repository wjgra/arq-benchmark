#ifndef _ARQ_TRANSMITTER_HPP_
#define _ARQ_TRANSMITTER_HPP_

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
        endOfTxSeqNum_{std::nullopt}
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
    }

    void sendPacket(arq::DataPacket&& packet) { inputBuffer_.addPacket(std::move(packet)); }

private:
    // The transmit thread handles transmission and retransmission of all packets. It continues
    // running until an end of transmission (EoT) packet has been transmitted and acknowledged.
    void transmitThread()
    {
        util::logInfo("Transmitter Tx thread started");

        // Lambda to check check output of the transmit function
        auto validateTx = [](decltype(std::function{txFn_})::result_type result) {
            if (result.has_value()) {
                util::logDebug("Successfully transmitted {} bytes", result.value());
            }
            else {
                util::logError("Transmit function failed!");
            }
        };

        while (!endOfTxSeqNum_.has_value() || retransmissionBuffer_->packetsPending()) {
            // WJG: If an ACK is received for a packet during retransmission, the packet can
            // be freed whilst transmission is in progress. Consider ownership (shared_ptr?).
            auto pkt = retransmissionBuffer_->getPacketDataSpan();

            if (pkt.has_value()) {
                DataPacketHeader hdr;
                hdr.deserialise(pkt.value());

                util::logInfo("Retransmitting packet with SN {} and length {}", hdr.sequenceNumber_, hdr.length_);
                auto ret = txFn_(pkt.value());
                validateTx(ret);
            }
            else if (!endOfTxSeqNum_.has_value() && retransmissionBuffer_->readyForNewPacket()) {
                // Need to mutex protect this so we're still ready for a new packet by the time we add one
                auto nextPkt = inputBuffer_.getPacket();
                if (nextPkt.isEndOfTx()) {
                    util::logInfo("Transmitter received end of EndofTx");
                    endOfTxSeqNum_ = nextPkt.info_.sequenceNumber_;
                }

                util::logInfo("Transmitting packet with SN {} and adding to retransmission buffer",
                              nextPkt.info_.sequenceNumber_);
                auto ret = txFn_(nextPkt.packet_.getReadSpan());
                validateTx(ret);

                retransmissionBuffer_->addPacket(std::move(nextPkt));
            }
        }
        util::logInfo("Transmitter Tx thread exited");
    }

    // The acknowledgement thread handles ACKs received at the transmitter from the receiver. It runs until
    // an ACK is received for an EoT packet.
    void ackThread()
    {
        util::logInfo("Transmitter ACK thread started");

        std::array<std::byte, arq::MAX_TRANSMISSION_UNIT> recvBuffer;
        while (true) { // WJG: add timeout if no ACKs Rx'd in certain window after EoT has been Tx'd
            auto ret = rxFn_(recvBuffer);
            if (ret.has_value()) {
                arq::SequenceNumber rxedSeqNum;
                if (arq::deserialiseSeqNum(rxedSeqNum, recvBuffer)) {
                    util::logInfo("Received ACK for SN {}", rxedSeqNum);
                    retransmissionBuffer_->acknowledgePacket(rxedSeqNum);

                    if (rxedSeqNum == endOfTxSeqNum_) {
                        util::logInfo("Received ACK that corresponds to end of transmission");
                        break;
                    }
                }
                else {
                    util::logWarning("Received packet that is too short to be an ACK");
                }
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
    // Thread handling data packet transmission
    std::thread transmitThread_;
    // Thread handling control packet reception
    std::thread ackThread_;
    // If an EoT has been received, store the SN here
    std::optional<SequenceNumber> endOfTxSeqNum_;
};

} // namespace arq

#endif
